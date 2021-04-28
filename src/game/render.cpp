#include "engine/camera.hpp"
#include "engine/game.hpp"
#include "utils/cache.hpp"
#include "utils/error.hpp"
#include "utils/file.hpp"
#include "utils/timeit.hpp"
#include <engine/components/ChunkData.hpp>

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <memory>
#include <string>

#include "engine/camera.hpp" // include last, it undefs near and far

extern engine::Camera g_camera;
extern int g_render_distance_horizontal;
extern int g_render_distance_vertical;

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
    if (!fp)
        utils::show_error("Can't open shader: assets/shaders/basic.vert"sv);
    std::string const vertex_shader_source_str = utils::load_file(fp);
    std::fclose(fp);
    fp = std::fopen("assets/shaders/basic.frag", "r");
    if (!fp)
        utils::show_error("Can't open shader: assets/shaders/basic.frag"sv);
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

void engine::Game::setup_texture()
{
    {
        utils::TimeIt timer { "load_textures"sv };
        m_textures = engine::load_textures();
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_textures.texture2d_array);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
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

    glm::mat4 const projection_matrix = glm::perspective(glm::radians(g_camera.fov), viewport[2] / static_cast<float>(viewport[3]), g_camera.near, g_camera.far);
    glm::vec3 const actual_position = glm::vec3 { -g_camera.position.x, g_camera.position.y, g_camera.position.z };
    glm::mat4 const view_matrix = glm::lookAt(actual_position, actual_position + g_camera.forward, g_camera.up);

    glUniformMatrix4fv(m_projection_uniform, 1, false, glm::value_ptr(projection_matrix));
    glUniformMatrix4fv(m_view_uniform, 1, false, glm::value_ptr(view_matrix));

    glm::i32vec3 player_chunk = g_camera.position / static_cast<float>(engine::C_ChunkData::chunk_size);

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
        glBindBuffer(GL_ARRAY_BUFFER, meshes.second.vertex_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshes.second.index_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, uv));
        glVertexAttribPointer(2, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, color));
        glVertexAttribPointer(3, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, light));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glDrawElements(GL_TRIANGLES, meshes.second.index_count, GL_UNSIGNED_INT, nullptr);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    for (auto const &[p, meshes] : m_chunk_meshes) {
        glBindBuffer(GL_ARRAY_BUFFER, meshes.first.vertex_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshes.first.index_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, uv));
        glVertexAttribPointer(2, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, color));
        glVertexAttribPointer(3, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(rendering::Vertex), (void *)offsetof(rendering::Vertex, light));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glDrawElements(GL_TRIANGLES, meshes.first.index_count, GL_UNSIGNED_INT, nullptr);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glUseProgram(0);
}
