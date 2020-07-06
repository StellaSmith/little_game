#include "engine/game.hpp"

#include <SDL.h>
#include <stb_image.h>

#include <string>
#include <fstream>
#include <cstdlib>
#include <string_view>

using namespace std::literals;

static std::string load_file(std::istream &is)
{
    return {std::istreambuf_iterator<char>{is}, std::istreambuf_iterator<char>{}};
}

static std::string load_file(std::string_view path)
{
    std::ifstream stream;
    stream.open(path.data());
    if (!stream.is_open())
        throw std::runtime_error("Can't open file");
    return load_file(stream);
}

void engine::Game::setup_shader()
{

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    std::string const vertex_shader_source_str = load_file("assets/basic.vert"sv);
    std::string const fragment_shader_source_str = load_file("assets/basic.frag"sv);

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
    if (!success)
    {
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &info_log_size);
        info_log.clear();
        info_log.resize(info_log_size);
        glGetShaderInfoLog(vertex_shader, info_log_size, nullptr, info_log.data());
        SDL_Log("OpenGL: Error compiling vertex shader: %s", info_log.c_str());
        std::exit(EXIT_FAILURE);
    }

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &info_log_size);
        info_log.clear();
        info_log.resize(info_log_size);
        glGetShaderInfoLog(vertex_shader, info_log_size, nullptr, info_log.data());
        SDL_Log("OpenGL: Error compiling fragment shader: %s", info_log.c_str());
        std::exit(EXIT_FAILURE);
    }

    m_shader = glCreateProgram();

    glAttachShader(m_shader, vertex_shader);
    glAttachShader(m_shader, fragment_shader);
    glLinkProgram(m_shader);

    glGetProgramiv(m_shader, GL_LINK_STATUS, &success);
    if (!success)
    {

        glGetProgramiv(m_shader, GL_INFO_LOG_LENGTH, &info_log_size);
        info_log.clear();
        info_log.resize(info_log_size);
        glGetProgramInfoLog(m_shader, info_log_size, nullptr, info_log.data());
        SDL_LogCritical(SDL_LOG_CATEGORY_RENDER, "OpenGL: Error linking shader:\n%s", info_log.c_str());
        std::exit(EXIT_FAILURE);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

void engine::Game::setup_texture()
{
    int x, y, c;
    unsigned char *data = stbi_load("assets/Phosphophyllite.jpg", &x, &y, &c, 4);

    if (!data)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "STB: Can't load image: %s", stbi_failure_reason());
        std::exit(EXIT_FAILURE);
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void engine::Game::start()
{
    glClearColor(0.0, 0.25, 0.5, 1.0);

    setup_shader();
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    setup_texture();

    glUseProgram(m_shader);
    glUniform1i(glGetUniformLocation(m_shader, "texture0"), 0);
    glUseProgram(0);

    running = true;
    {
        chunk_t chunk;
        chunk.blocks[0].id = 1;
        chunk.modified = true;

        m_chunks.emplace(glm::u32vec4{0, 0, 0, 0}, std::move(chunk));
    }
}

void engine::Game::stop()
{
    running = false;
}

void engine::Game::input(SDL_Event const &event)
{
    if (event.type == SDL_QUIT)
        stop();
}

void engine::Game::update(engine::Game::clock_type::duration delta)
{
    std::vector<glm::i32vec4> to_delete;
    std::vector<chunk_t> to_add;
    for (auto &[k, chunk] : m_chunks)
    {
        if (chunk.modified)
        {

            auto it = m_chunk_meshes.find(chunk.position);
            if (it == m_chunk_meshes.end())
            {
                GLuint buffers[2];
                glGenBuffers(2, buffers);

                std::tie(it, std::ignore) = m_chunk_meshes.emplace(chunk.position, chunk_renderer::chunk_meshes{buffers[0], buffers[1], 0, 0});
            }

            engine::chunk_renderer::generate_mesh(chunk, it->second);

            chunk.modified = false;
        }
    }

    for (auto pos : to_delete)
        m_chunks.erase(pos);

    for (auto &chunk : to_add)
        m_chunks.emplace(chunk.position, std::move(chunk));
}

void engine::Game::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glUseProgram(m_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glBindVertexArray(m_vao);

    for (auto const &[p, meshes] : m_chunk_meshes)
    {
        glBindBuffer(GL_ARRAY_BUFFER, meshes.solid_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 20, (void *)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 20, (void *)12);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glDrawArrays(GL_TRIANGLES, 0, meshes.solid_vertices);

        glBindBuffer(GL_ARRAY_BUFFER, meshes.translucent_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 20, (void *)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 20, (void *)12);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glDrawArrays(GL_TRIANGLES, 0, meshes.translucent_vertices);
    }
}