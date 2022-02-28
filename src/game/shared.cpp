#include <engine/BlockType.hpp>
#include <engine/Camera.hpp>
#include <engine/Game.hpp>
#include <engine/components/ChunkData.hpp>
#include <engine/components/ChunkPosition.hpp>
#include <engine/components/Dirty.hpp>
#include <math/bits.hpp>

#include <entt/entt.hpp>
#include <glad/glad.h>
#include <lua.hpp>

#include <cstdint>
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
    setup_lua();

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

    m_entity_registry.on_construct<engine::C_ChunkPosition>().connect<&Game::on_chunk_construct>(*this);
    m_entity_registry.on_destroy<engine::C_ChunkPosition>().connect<&Game::on_chunk_destroy>(*this);

    std::uint32_t const colorful_id = block_type_names().find("colorful_block"sv)->second;

    running = true;
    std::random_device rd {};
    std::uniform_int_distribution<std::uint16_t> dist { 0, 255 };
    std::uniform_int_distribution<std::size_t> id_dist { 0, m_block_types.size() };

    int32_t const max_x = 10;
    for (std::int32_t x = 0; x < max_x; ++x) {
        auto chunk = m_entity_registry.create();
        m_entity_registry.emplace<engine::C_ChunkPosition>(chunk, x - max_x / 2);
        auto &chunk_data = m_entity_registry.emplace<engine::C_ChunkData>(chunk);
        m_entity_registry.emplace<engine::C_Dirty>(chunk);

        for (auto &block : chunk_data.blocks) {
            if ((block.type = id_dist(rd)) == colorful_id) {
                block.data = math::pack_u32(dist(rd), dist(rd), dist(rd));
            }
        }
    }
}
#include <spdlog/spdlog.h>
void engine::Game::on_chunk_construct(entt::registry &registry, entt::entity chunk)
{
    assert(&m_entity_registry == &registry); // sanity check
    glm::i32vec4 chunk_position = registry.get<engine::C_ChunkPosition>(chunk);
    m_chunks.emplace(chunk_position, chunk);
}

void engine::Game::on_chunk_destroy(entt::registry &registry, entt::entity chunk)
{
    assert(&m_entity_registry == &registry); // sanity check
    glm::i32vec4 chunk_position = registry.get<engine::C_ChunkPosition>(chunk);
    m_chunks.erase(chunk_position);
}

void engine::Game::stop()
{
    running = false;
}

void engine::Game::cleanup()
{
    assert(!running);
    {
        std::vector<entt::entity> const to_delete(m_entity_registry.data(), m_entity_registry.data() + m_entity_registry.size());
        m_entity_registry.destroy(to_delete.cbegin(), to_delete.cend());
    }
    m_chunk_meshes.clear();
    m_translucent_mesh_data.clear();
}