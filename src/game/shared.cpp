#include "engine/camera.hpp"
#include "engine/chunk_t.hpp"
#include "engine/game.hpp"
#include "math/bits.hpp"

#include <glad/glad.h>

#include <random>

engine::Camera g_camera;

engine::Game::~Game()
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteProgram(m_shader);
    glDeleteTextures(1, &m_textures.texture2d_array);
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
    std::random_device rd {};
    std::uniform_int_distribution<std::uint16_t> dist { 0, 255 };
    std::uniform_int_distribution<std::uint32_t> id_dist { 1, 4 };

    int32_t const max_x = 10;
    for (std::int32_t x = 0; x < max_x; ++x) {
        chunk_t chunk {};

        for (auto &block : chunk.blocks) {
            if ((block.id = id_dist(rd)) == 1)
                block.data.u64 = math::pack_u32(dist(rd), dist(rd), dist(rd));
        }

        chunk.blocks[0].id = 1;
        chunk.blocks[0].data.u64 = math::pack_u32(0xff, 0xff, 0xff);
        chunk.blocks[1].id = 2;
        chunk.blocks[2].id = 3;
        chunk.blocks[3].id = 4;

        chunk.modified = true;
        chunk.position = glm::i32vec4 { x - max_x / 2, 0, 0, 0 };

        m_chunks.emplace(glm::i32vec4 { x - max_x / 2, 0, 0, 0 }, std::move(chunk));
    }
}

void engine::Game::stop()
{
    running = false;
}