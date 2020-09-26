#include "engine/game.hpp"
#include "engine/chunk_mesh_generation.hpp"
#include "math/bits.hpp"
#include "utils/cache.hpp"
#include "utils/file.hpp"

#include <SDL.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <stb_image.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <string_view>

using namespace std::literals;

void engine::Game::setup_shader()
{

#ifdef GL_ARB_get_program_binary
    if (GLAD_GL_ARB_get_program_binary) {
        std::FILE *fp = utils::get_cache_file("shaders/basic.bin"sv, { "assets/basic.vert"sv, "assets/basic.frag"sv });
        if (fp) {
            GLenum format;
            std::fread(&format, sizeof(format), 1, fp);
            auto start = std::ftell(fp);
            std::fseek(fp, 0, SEEK_END);
            auto stop = std::ftell(fp);
            auto len = static_cast<std::size_t>(stop - start);
            std::fseek(fp, start, SEEK_SET);
            auto data = std::make_unique<std::byte[]>(len);
            std::fread(data.get(), 1, len, fp);
            std::fclose(fp);

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

    std::FILE *fp = std::fopen("assets/shaders/basic.vert", "r");
    if (!fp) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Can't open shader: assets/shaders/basic.vert");
        std::exit(EXIT_FAILURE);
    }
    std::string const vertex_shader_source_str = utils::load_file(fp);
    std::fclose(fp);
    fp = std::fopen("assets/shaders/basic.frag", "r");
    if (!fp) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Can't open shader: assets/shaders/basic.frag");
        std::exit(EXIT_FAILURE);
    }
    std::string const fragment_shader_source_str = utils::load_file(fp);
    std::fclose(fp);

    char const *const vertex_shader_source = vertex_shader_source_str.c_str();
    char const *const fragment_shader_source = fragment_shader_source_str.c_str();

    glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);

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
        SDL_Log("OpenGL: Error compiling vertex shader: %s", info_log.c_str());
        std::exit(EXIT_FAILURE);
    }

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &info_log_size);
        info_log.clear();
        info_log.resize(info_log_size);
        glGetShaderInfoLog(fragment_shader, info_log_size, nullptr, info_log.data());
        SDL_Log("OpenGL: Error compiling fragment shader: %s", info_log.c_str());
        std::exit(EXIT_FAILURE);
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
        SDL_LogCritical(SDL_LOG_CATEGORY_RENDER, "OpenGL: Error linking shader:\n%s", info_log.c_str());
        std::exit(EXIT_FAILURE);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

#ifdef GL_ARB_get_program_binary
    if (GLAD_GL_ARB_get_program_binary) {
        std::FILE *fp = utils::create_cache_file("shaders/basic.bin");
        if (!fp)
            return;
        GLint bufSize;
        glGetProgramiv(m_shader, GL_PROGRAM_BINARY_LENGTH, &bufSize);
        auto data = std::make_unique<std::byte[]>(bufSize);
        GLsizei length = bufSize;
        GLenum format;
        glGetProgramBinary(m_shader, bufSize, &length, &format, data.get());

        std::fwrite(&format, sizeof(format), 1, fp);
        std::fwrite(data.get(), length, 1, fp);
        std::fclose(fp);
    }
#endif
}
#include "utils/timeit.hpp"
void engine::Game::setup_texture()
{
    {
        utils::TimeIt timer { "load_textures"sv };
        m_textures = engine::load_textures();
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_textures.texture2d_array);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void engine::Game::start()
{
    glClearColor(0.0, 0.25, 0.5, 1.0);

    setup_shader();
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    setup_texture();

    glUseProgram(m_shader);
    m_projection_uniform = glGetUniformLocation(m_shader, "projection");
    m_view_uniform = glGetUniformLocation(m_shader, "view");

    glUniform1i(glGetUniformLocation(m_shader, "texture0"), 0);
    glUseProgram(0);

    running = true;
    {
        chunk_t chunk {};

        // std::random_device rd {};
        // std::uniform_int_distribution<std::uint16_t> dist { 0, 255 };
        // for (auto &block : chunk.blocks) {
        //     block.id = 1;
        //     block.data.u64 = math::pack_u32(dist(rd), dist(rd), dist(rd));
        // }

        chunk.blocks[0].id = 1;
        chunk.blocks[0].data.u64 = math::pack_u32(0xff, 0xff, 0xff);
        chunk.blocks[1].id = 2;
        chunk.blocks[2].id = 3;
        chunk.blocks[3].id = 4;

        chunk.modified = true;
        chunk.position = glm::ivec4 { 0, 0, 0, 0 };

        m_chunks.emplace(glm::u32vec4 { 0, 0, 0, 0 }, std::move(chunk));
    }
}

void engine::Game::stop()
{
    running = false;
}

struct Camera {
    glm::vec3 position = glm::vec3 { 0.0f, 0.0f, -1.0f };
    glm::vec3 forward = glm::vec3 { 0.0f, 0.0f, 1.0f };
    float fov = 60.0f;
    bool to_center;
} g_camera;

void engine::Game::input(SDL_Event const &event)
{
    if (event.type == SDL_QUIT)
        stop();
    else if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
            glViewport(0, 0, event.window.data1, event.window.data2);
        }
    }
}

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

static std::vector<std::uint32_t> get_sorted_indices(std::vector<engine::rendering::block_vertex_t> const &vertices, std::vector<std::uint32_t> indices)
{
    using face_t = std::array<std::uint32_t, 3>;
    auto begin = reinterpret_cast<face_t *>(indices.data());
    auto end = reinterpret_cast<face_t *>(indices.data() + indices.size());

    std::sort(begin, end, [&vertices](face_t const &lhs, face_t const &rhs) {
        auto const lhs_center = (vertices[lhs[0]].position + vertices[lhs[1]].position + vertices[lhs[2]].position) / 3.0f;
        auto const rhs_center = (vertices[rhs[0]].position + vertices[rhs[1]].position + vertices[rhs[2]].position) / 3.0f;
        auto const lhs_distance = glm::length(lhs_center - g_camera.position);
        auto const rhs_distance = glm::length(rhs_center - g_camera.position);
        return lhs_distance < rhs_distance;
    });

    return indices;
}

static glm::vec3 previous_camera_position {};

void engine::Game::update([[maybe_unused]] engine::Game::clock_type::duration delta)
{
    if (ImGui::Begin("Camera Controls")) {
        ImGui::SliderFloat("FOV", &g_camera.fov, 30.0f, 130.0f);
        ImGui::SliderFloat3("Position", glm::value_ptr(g_camera.position), -10.0f, 10.0f);
        ImGui::Checkbox("Looking at center", &g_camera.to_center);
        if (ImGui::Button("Reset")) {
            g_camera.position = glm::vec3 { 0.0f, 0.0f, -1.0f };
            g_camera.forward = glm::vec3 { 0.0f, 0.0f, 1.0f };
            g_camera.fov = 60.0f;
            g_camera.to_center = false;
        }
    }
    ImGui::End();
    if (ImGui::Begin("Random Stuff")) {
        static float col[3] { 0, 0, 0 };
        if (ImGui::ColorPicker3("Colorful block color", col)) {
            chunk_t &chunk = m_chunks.find(glm::i32vec4 { 0, 0, 0, 0 })->second;
            chunk.modified = true;
            chunk.blocks[0].data.u64 = math::pack_u32(col[0] * 255.0f, col[1] * 255.0f, col[2] * 255.0f);
            ImGui::Text("Chunk Modified");
        }
    }
    ImGui::End();

    std::vector<glm::i32vec4> to_delete;
    std::vector<chunk_t> to_add;
    for (auto &[k, chunk] : m_chunks) {
        if (chunk.modified) {
            auto it = std::find_if(m_chunk_meshes.begin(), m_chunk_meshes.end(), [&pos = chunk.position](auto const &p) { return p.first == pos; });
            //auto it = m_chunk_meshes.find(chunk.position);
            if (it == m_chunk_meshes.end()) {
                GLuint buffers[4];
                glGenBuffers(std::size(buffers), buffers);

                m_chunk_meshes.emplace_back(chunk.position, rendering::chunk_meshes { buffers[0], buffers[1], buffers[2], buffers[3], 0, 0 });
                it = m_chunk_meshes.end() - 1;
            }

            auto const solid_mesh = generate_solid_mesh(*this, chunk);
            auto const translucent_mesh = generate_translucent_mesh(*this, chunk);

            auto const sorted_indices = get_sorted_indices(translucent_mesh.vertices, translucent_mesh.indices);

            // TODO: Vertex deduplication

            glBindBuffer(GL_ARRAY_BUFFER, it->second.solid_vertex_buffer);
            glBufferData(GL_ARRAY_BUFFER, solid_mesh.vertices.size() * sizeof(*solid_mesh.vertices.data()), solid_mesh.vertices.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->second.solid_index_buffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, solid_mesh.indices.size() * sizeof(*solid_mesh.indices.data()), solid_mesh.indices.data(), GL_DYNAMIC_DRAW);
            it->second.solid_index_count = solid_mesh.indices.size();

            glBindBuffer(GL_ARRAY_BUFFER, it->second.translucent_vertex_buffer);
            glBufferData(GL_ARRAY_BUFFER, translucent_mesh.vertices.size() * sizeof(*translucent_mesh.vertices.data()), translucent_mesh.vertices.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->second.translucent_index_buffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sorted_indices.size() * sizeof(*sorted_indices.data()), sorted_indices.data(), GL_STREAM_DRAW);
            it->second.translucent_index_count = sorted_indices.size();

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            if (auto mesh_data = m_translucent_mesh_data.find(chunk.position); mesh_data != m_translucent_mesh_data.end())
                mesh_data->second = std::move(translucent_mesh);
            else
                m_translucent_mesh_data.emplace(chunk.position, std::move(translucent_mesh));

            chunk.modified = false;
        } else if (g_camera.position != previous_camera_position) {
            auto p_mesh = std::find_if(m_chunk_meshes.begin(), m_chunk_meshes.end(), [&position = chunk.position](auto const &p) { return p.first == position; });
            auto p_data = m_translucent_mesh_data.find(chunk.position);
            if (p_mesh != m_chunk_meshes.end() && p_data != m_translucent_mesh_data.end()) {
                auto const sorted_indices = get_sorted_indices(p_data->second.vertices, p_data->second.indices);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p_mesh->second.translucent_index_buffer);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sorted_indices.size() * sizeof(*sorted_indices.data()), sorted_indices.data(), GL_STREAM_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }
        }
    }

    for (auto position : to_delete) {
        m_chunks.erase(position);
        m_translucent_mesh_data.erase(position);
        auto p_mesh = std::find_if(m_chunk_meshes.begin(), m_chunk_meshes.end(), [position](auto const &p) { return p.first == position; });
        if (p_mesh != m_chunk_meshes.end())
            m_chunk_meshes.erase(p_mesh);
    }

    for (auto &chunk : to_add) {
        chunk.modified = true; // will generate the meshes next iteration
        m_chunks.emplace(chunk.position, std::move(chunk));
    }

    previous_camera_position = g_camera.position;
}

void engine::Game::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glUseProgram(m_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_textures.texture2d_array);
    glBindVertexArray(m_vao);

    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glm::mat4 const projection_matrix = glm::perspective(glm::radians(g_camera.fov), viewport[2] / (float)viewport[3], 0.1f, 100.0f);
    glm::vec3 const actual_position = glm::vec3 { -g_camera.position.x, g_camera.position.y, g_camera.position.z };
    glm::mat4 const view_matrix = glm::lookAt(actual_position, (actual_position + g_camera.forward) * static_cast<float>(!g_camera.to_center), glm::vec3 { 0.0f, 1.0f, 0.0f });

    glUniformMatrix4fv(m_projection_uniform, 1, false, glm::value_ptr(projection_matrix));
    glUniformMatrix4fv(m_view_uniform, 1, false, glm::value_ptr(view_matrix));

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    for (auto const &[p, meshes] : m_chunk_meshes) {
        glBindBuffer(GL_ARRAY_BUFFER, meshes.translucent_vertex_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshes.translucent_index_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rendering::block_vertex_t), (void *)offsetof(rendering::block_vertex_t, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(rendering::block_vertex_t), (void *)offsetof(rendering::block_vertex_t, uv));
        glVertexAttribPointer(2, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rendering::block_vertex_t), (void *)offsetof(rendering::block_vertex_t, color));
        glVertexAttribPointer(3, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rendering::block_vertex_t), (void *)offsetof(rendering::block_vertex_t, light));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glDrawElements(GL_TRIANGLES, meshes.translucent_index_count, GL_UNSIGNED_INT, nullptr);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    for (auto const &[p, meshes] : m_chunk_meshes) {
        glBindBuffer(GL_ARRAY_BUFFER, meshes.solid_vertex_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshes.solid_index_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rendering::block_vertex_t), (void *)offsetof(rendering::block_vertex_t, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,sizeof(rendering::block_vertex_t), (void *)offsetof(rendering::block_vertex_t, uv));
        glVertexAttribPointer(2, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rendering::block_vertex_t), (void *)offsetof(rendering::block_vertex_t, color));
        glVertexAttribPointer(3, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rendering::block_vertex_t), (void *)offsetof(rendering::block_vertex_t, light));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glDrawElements(GL_TRIANGLES, meshes.solid_index_count, GL_UNSIGNED_INT, nullptr);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}
