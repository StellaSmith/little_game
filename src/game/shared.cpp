#include "engine/Chunk.hpp"
#include "engine/camera.hpp"
#include "engine/game.hpp"
#include "math/bits.hpp"
#include <engine/BlockType.hpp>

#include <glad/gl.h>

#include <random>

using namespace std::literals;

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

    auto registered_blocks_types = BlockType::GetRegistered();
    for (auto *block_type : registered_blocks_types)
        if (block_type->initialize)
            block_type->initialize(block_type, this);

    std::uint32_t colorfulId = BlockType::GetRegisteredIdByName("colorful_block"sv);

    running = true;
    std::random_device rd {};
    std::uniform_int_distribution<std::uint16_t> dist { 0, 255 };
    std::uniform_int_distribution<std::uint32_t> id_dist { 0, static_cast<std::uint32_t>(registered_blocks_types.size() - 1) };

    int32_t const max_x = 10;
    for (std::int32_t x = 0; x < max_x; ++x) {
        Chunk chunk {};

        for (auto &block : chunk.blocks) {
            if ((block.id = id_dist(rd)) == colorfulId)
                block.data.u64 = math::pack_u32(dist(rd), dist(rd), dist(rd));
        }

        chunk.modified = true;
        chunk.position = glm::i32vec4 { x - max_x / 2, 0, 0, 0 };

        m_chunks.emplace(glm::i32vec4 { x - max_x / 2, 0, 0, 0 }, std::move(chunk));
    }
}

void engine::Game::stop()
{
    running = false;
}