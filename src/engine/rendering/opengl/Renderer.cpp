#include "engine/File.hpp"
#include <engine/Camera.hpp>
#include <engine/Config.hpp>
#include <engine/Game.hpp>
#include <engine/cache.hpp>
#include <engine/rendering/opengl/Renderer.hpp>

#include <SDL_mouse.h>
#include <glad/glad.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <imgui.h>
#include <utils/error.hpp>
#include <utils/file.hpp>

extern engine::Camera g_camera;
extern int g_render_distance_horizontal;
extern int g_render_distance_vertical;

using namespace std::literals;

engine::sdl::Window engine::rendering::opengl::Renderer::create_window(const char *title, int x, int y, int w, int h, uint32_t flags)
{

    auto const &config = engine::config();

    SDL_GL_ResetAttributes();
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    if (config.opengl.red_bits) SDL_GL_SetAttribute(SDL_GL_RED_SIZE, *config.opengl.red_bits);
    if (config.opengl.green_bits) SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, *config.opengl.green_bits);
    if (config.opengl.blue_bits) SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, *config.opengl.blue_bits);
    if (config.opengl.alpha_bits) SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, *config.opengl.alpha_bits);
    if (config.opengl.depth_bits) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, *config.opengl.depth_bits);
    if (config.opengl.stencil_bits) SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, *config.opengl.stencil_bits);

    auto window = engine::sdl::Window::create(title, glm::ivec2 { x, y }, glm::ivec2 { w, h }, flags | SDL_WINDOW_OPENGL);

#ifndef NDEBUG
    if (SDL_SetRelativeMouseMode(SDL_TRUE))
        utils::show_error("Can't set mouse to relative mode!"sv);
#endif
    return window;
}

void engine::rendering::opengl::Renderer::setup()
{
    m_context = game().window().opengl().create_context();
    m_context->make_current();

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&SDL_GL_GetProcAddress)))
        utils::show_error("GLAD Error."sv, "Failed to initialize the OpenGL context."sv);

    int major, minor, r, g, b, a, d, s;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &r);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &b);
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &d);
    SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &s);

    char const *version = reinterpret_cast<char const *>(glGetString(GL_VERSION));
    char const *vendor = reinterpret_cast<char const *>(glGetString(GL_VENDOR));
    char const *renderer = reinterpret_cast<char const *>(glGetString(GL_RENDERER));
    char const *shading_version = reinterpret_cast<char const *>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    SPDLOG_INFO("OpenGL profile: {}.{}", major, minor);
    SPDLOG_INFO("\tRGBA bits: {}, {}, {}, {}", r, g, b, a);
    SPDLOG_INFO("\tDepth bits: {}", d);
    SPDLOG_INFO("\tStencil bits: {}", s);
    SPDLOG_INFO("\tVersion: {}", version);
    SPDLOG_INFO("\tVendor: {}", vendor);
    SPDLOG_INFO("\tRendered: {}", renderer);
    SPDLOG_INFO("\tShading language version: {}", shading_version);

    SDL_GL_SetSwapInterval(0);

    glClearColor(0.0, 0.25, 0.5, 1.0);

    setup_shader();
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    setup_texture();

    glUseProgram(m_shader);
    m_uniforms.projection = glGetUniformLocation(m_shader, "projection");
    m_uniforms.view = glGetUniformLocation(m_shader, "view");

    glUniform1i(glGetUniformLocation(m_shader, "texture0"), 0);
    glUseProgram(0);
}

#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

void engine::rendering::opengl::Renderer::imgui_setup()
{
    ImGui_ImplSDL2_InitForOpenGL(game().window().get(), m_context->get()); // always returns true

    // See imgui/examples/imgui_impl_opengl3.cpp
    ImGui_ImplOpenGL3_Init("#version 330 core"); // always returns true
}

void engine::rendering::opengl::Renderer::imgui_new_frame(std::shared_ptr<engine::rendering::IRenderTarget> target)
{
    (void)target;
    ImGui_ImplOpenGL3_NewFrame();
}

void engine::rendering::opengl::Renderer::setup_shader()
{
#ifdef GL_ARB_get_program_binary
    if (GLAD_GL_ARB_get_program_binary) {
        static std::filesystem::path const ref_files[] = { "assets/terrain/basic.vert", "assets/terrain/basic.frag" };
        auto fp = engine::get_cache_file("shaders/terrain/basic.bin"sv, ref_files);
        if (fp) {
            GLenum format;
            std::fread(&format, sizeof(format), 1, fp.assume_value().get());
            auto start = std::ftell(fp.assume_value().get());
            std::fseek(fp.assume_value().get(), 0, SEEK_END);
            auto stop = std::ftell(fp.assume_value().get());
            auto len = static_cast<std::size_t>(stop - start);
            std::fseek(fp.assume_value().get(), start, SEEK_SET);
            auto data = std::make_unique<std::byte[]>(len);
            std::fread(data.get(), 1, len, fp.assume_value().get());

            m_shader = glCreateProgram();
            glProgramBinary(m_shader, format, data.get(), len);

            int success;
            glGetProgramiv(m_shader, GL_LINK_STATUS, &success);
            if (success)
                return;
            else
                glDeleteProgram(m_shader);
        }
    }
#endif

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    auto const vertex_shader_source = engine::File::open("assets/shaders/terrain/basic.vert", "r")
                                          .bytes()
                                          .string();
    auto const fragment_shader_source = engine::File::open("assets/shaders/terrain/basic.frag", "r")
                                            .bytes()
                                            .string();

    auto const vertex_shader_source_raw = vertex_shader_source.c_str();
    auto const fragment_shader_source_raw = fragment_shader_source.c_str();

    glShaderSource(vertex_shader, 1, &vertex_shader_source_raw, nullptr);
    glShaderSource(fragment_shader, 1, &fragment_shader_source_raw, nullptr);

    glCompileShader(vertex_shader);
    glCompileShader(fragment_shader);

    int success;
    std::string info_log;
    GLint info_log_size;

    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &info_log_size);
        info_log.clear();
        info_log.resize(info_log_size);
        glGetShaderInfoLog(vertex_shader, info_log_size, nullptr, info_log.data());

        utils::show_error("OpenGL Error."sv, "Couldn't compile vertex shader\n"s + info_log);
    }

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &info_log_size);
        info_log.clear();
        info_log.resize(info_log_size);
        glGetShaderInfoLog(fragment_shader, info_log_size, nullptr, info_log.data());

        utils::show_error("OpenGL Error."sv, "Couldn't compile fragment shader\n"s + info_log);
    }

    m_shader = glCreateProgram();

    glAttachShader(m_shader, vertex_shader);
    glAttachShader(m_shader, fragment_shader);
    glLinkProgram(m_shader);

    glGetProgramiv(m_shader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramiv(m_shader, GL_INFO_LOG_LENGTH, &info_log_size);
        info_log.clear();
        info_log.resize(info_log_size);
        glGetProgramInfoLog(m_shader, info_log_size, nullptr, info_log.data());

        utils::show_error("OpenGL Error."sv, "Couldn't link shader\n"s + info_log);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

#ifdef GL_ARB_get_program_binary
    if (GLAD_GL_ARB_get_program_binary) {
        auto fp = engine::create_cache_file("shaders/terrain/basic.bin");
        if (!fp)
            return;
        GLint buf_len;
        glGetProgramiv(m_shader, GL_PROGRAM_BINARY_LENGTH, &buf_len);
        auto data = std::make_unique<std::byte[]>(buf_len);
        GLsizei length = buf_len;
        GLenum format;
        glGetProgramBinary(m_shader, buf_len, &length, &format, data.get());

        std::fwrite(&format, sizeof(format), 1, fp.assume_value().get());
        std::fwrite(data.get(), length, 1, fp.assume_value().get());
    }
#endif
}

#include <engine/rendering/opengl/Texture.hpp>

void engine::rendering::opengl::Renderer::setup_texture()
{
    auto texture = opengl::Texture();
    texture.create();
    texture.bind(GL_TEXTURE_2D_ARRAY)
        .setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
        .setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
        .setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)
        .setParameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void engine::rendering::opengl::Renderer::render(float)
{

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glUseProgram(m_shader);
    glActiveTexture(GL_TEXTURE0);
#if 0
    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer.textures.texture2d_array);
#endif
    glBindVertexArray(m_vao);

    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glm::mat4 const projection_matrix = glm::perspective(glm::radians(g_camera.fov), viewport[2] / static_cast<float>(viewport[3]), g_camera.near_plane, g_camera.far_plane);
    glm::vec3 const actual_position = glm::vec3 { -g_camera.position.x, g_camera.position.y, g_camera.position.z };
    glm::mat4 const view_matrix = glm::lookAt(actual_position, actual_position + g_camera.forward, g_camera.up);

    glUniformMatrix4fv(m_uniforms.projection, 1, false, glm::value_ptr(projection_matrix));
    glUniformMatrix4fv(m_uniforms.view, 1, false, glm::value_ptr(view_matrix));

    glm::i32vec3 player_chunk = g_camera.position / static_cast<float>(engine::components::ChunkData::chunk_size);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    for (auto const &[position, meshes] : m_chunk_meshes) {
        if (position.x < player_chunk.x - g_render_distance_horizontal
            || position.x > player_chunk.x + g_render_distance_horizontal
            || position.y < player_chunk.y - g_render_distance_vertical
            || position.y > player_chunk.y + g_render_distance_vertical
            || position.z < player_chunk.z - g_render_distance_horizontal
            || position.z > player_chunk.z + g_render_distance_horizontal)
            continue;
        glBindBuffer(GL_ARRAY_BUFFER, meshes.solid_mesh.vertex_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshes.solid_mesh.index_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, uv));
        glVertexAttribPointer(2, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, color));
        glVertexAttribPointer(3, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, light));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glDrawElements(GL_TRIANGLES, meshes.translucent_mesh.index_count, GL_UNSIGNED_INT, nullptr);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    for (auto const &[p, meshes] : m_chunk_meshes) {
        glBindBuffer(GL_ARRAY_BUFFER, meshes.solid_mesh.vertex_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshes.solid_mesh.index_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, uv));
        glVertexAttribPointer(2, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, color));
        glVertexAttribPointer(3, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, light));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glDrawElements(GL_TRIANGLES, meshes.solid_mesh.index_count, GL_UNSIGNED_INT, nullptr);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glUseProgram(0);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // swap the buffer (present to the window surface)
    game().window().opengl().swap_buffers();
}

#include <engine/ecs/components/ChunkData.hpp>
#include <engine/ecs/components/ChunkPosition.hpp>
#include <engine/ecs/components/Dirty.hpp>

static auto get_sorted_indices(engine::rendering::Mesh const &mesh)
{
    auto result = mesh.indices;
    using face_t = std::array<std::uint32_t, 3>;
    auto begin = reinterpret_cast<face_t *>(result.data());
    auto end = reinterpret_cast<face_t *>(result.data() + result.size());

    std::sort(begin, end, [&vertices = mesh.vertices](face_t const &lhs, face_t const &rhs) {
        auto const lhs_center = (vertices[lhs[0]].position + vertices[lhs[1]].position + vertices[lhs[2]].position) / 3.0f;
        auto const rhs_center = (vertices[rhs[0]].position + vertices[rhs[1]].position + vertices[rhs[2]].position) / 3.0f;
        auto const lhs_distance = glm::length(lhs_center - g_camera.position);
        auto const rhs_distance = glm::length(rhs_center - g_camera.position);
        return lhs_distance < rhs_distance;
    });

    return result;
}

void engine::rendering::opengl::Renderer::update()
{
    ImGui_ImplOpenGL3_NewFrame();

    auto &registry = game().registry();
    registry.view<engine::components::ChunkPosition>(entt::exclude<engine::components::Dirty>).each([&](auto const &chunk_position) {
        auto p_mesh = m_chunk_meshes.find(chunk_position);
        auto p_data = m_translucent_mesh_data.find(chunk_position);
        if (p_mesh != m_chunk_meshes.end() && p_data != m_translucent_mesh_data.end()) {
            auto const sorted_indices = get_sorted_indices(p_data->second);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p_mesh->second.translucent_mesh.index_buffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sorted_indices.size() * sizeof(*sorted_indices.data()), sorted_indices.data(), GL_STREAM_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
    });

    registry.view<engine::components::ChunkPosition, engine::components::ChunkData, engine::components::Dirty>().each([&](entt::entity chunk, auto const &chunk_position, auto const &chunk_data) {
        auto it = m_chunk_meshes.find(chunk_position);
        if (it == m_chunk_meshes.end()) {

            GLuint buffers[4];
            glGenBuffers(std::size(buffers), buffers);
            std::tie(it, std::ignore) = m_chunk_meshes.emplace(chunk_position,
                Renderer::ChunkMeshes {
                    .translucent_mesh = rendering::opengl::MeshHandle { buffers[0], buffers[1], 0 },
                    .solid_mesh = rendering::opengl ::MeshHandle { buffers[2], buffers[3], 0 } });
        }

        auto const solid_mesh = game().generate_solid_mesh(chunk_position, chunk_data);
        auto const translucent_mesh = game().generate_translucent_mesh(chunk_position);

        auto const sorted_indices = get_sorted_indices(translucent_mesh);

        // TODO: Vertex deduplication?

        glBindBuffer(GL_ARRAY_BUFFER, it->second.solid_mesh.vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, solid_mesh.vertices.size() * sizeof(*solid_mesh.vertices.data()), solid_mesh.vertices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->second.solid_mesh.index_buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, solid_mesh.indices.size() * sizeof(*solid_mesh.indices.data()), solid_mesh.indices.data(), GL_DYNAMIC_DRAW);

        it->second.solid_mesh.index_count = solid_mesh.indices.size();

        glBindBuffer(GL_ARRAY_BUFFER, it->second.solid_mesh.vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, translucent_mesh.vertices.size() * sizeof(*translucent_mesh.vertices.data()), translucent_mesh.vertices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->second.solid_mesh.index_buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sorted_indices.size() * sizeof(*sorted_indices.data()), sorted_indices.data(), GL_STREAM_DRAW);

        it->second.solid_mesh.index_count = sorted_indices.size();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        if (auto mesh_data = m_translucent_mesh_data.find(chunk_position); mesh_data != m_translucent_mesh_data.end())
            mesh_data->second = std::move(translucent_mesh);
        else
            m_translucent_mesh_data.emplace(chunk_position, std::move(translucent_mesh));

        registry.remove<engine::components::Dirty>(chunk);
    });
}

engine::rendering::opengl::Renderer::~Renderer()
{
    ImGui_ImplOpenGL3_Shutdown();
    glDeleteVertexArrays(1, &m_vao);
    glDeleteProgram(m_shader);
#if 0
    glDeleteTextures(1, &m_textures.texture2d_array);
#endif
}
