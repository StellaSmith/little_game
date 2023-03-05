#include <engine/BlockType.hpp>
#include <engine/Camera.hpp>
#include <engine/Config.hpp>
#include <engine/Game.hpp>
#include <engine/components/ChunkData.hpp>
#include <engine/components/ChunkPosition.hpp>
#include <engine/components/Dirty.hpp>
#include <math/bits.hpp>

#include <entt/entt.hpp>
#ifdef ENGINE_WITH_OPENGL
#include <glad/glad.h>
#endif
#include <lua.hpp>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <optional>
#include <random>

using namespace std::literals;

engine::Camera g_camera;

engine::Game::~Game()
{
#ifdef ENGINE_WITH_OPENGL
    glDeleteVertexArrays(1, &m_vao);
    glDeleteProgram(m_shader);
    glDeleteTextures(1, &m_textures.texture2d_array);
#endif
}

void engine::Game::start()
{
    m_console_text.set_capacity(engine::config().terminal.max_lines);

    setup_lua();

#ifdef ENGINE_WITH_OPENGL
    setup_opengl();
#endif

    m_entity_registry.on_construct<engine::components::ChunkPosition>().connect<&Game::on_chunk_construct>(*this);
    m_entity_registry.on_destroy<engine::components::ChunkPosition>().connect<&Game::on_chunk_destroy>(*this);

    auto const maybe_colorful_id = m_block_registry.index("colorful_block");

    running = true;
    std::random_device rd {};
    std::uniform_int_distribution<std::uint16_t> color_dist { 0, 255 };
    std::uniform_int_distribution<std::size_t> id_dist { 0, m_block_registry.size() };

    int32_t const max_x = 10;
    for (std::int32_t x = 0; x < max_x; ++x) {
        auto chunk = m_entity_registry.create();
        m_entity_registry.emplace<engine::components::ChunkPosition>(chunk, x - max_x / 2);
        auto &chunk_data = m_entity_registry.emplace<engine::components::ChunkData>(chunk);
        m_entity_registry.emplace<engine::components::Dirty>(chunk);

        for (auto &block : chunk_data.blocks) {
            if (maybe_colorful_id != entt::null) {
                if ((block.type_id = static_cast<entt::id_type>(block_registry().storage()[id_dist(rd)])) == static_cast<entt::id_type>(maybe_colorful_id)) {
                    block.data_id = math::pack_u32(color_dist(rd), color_dist(rd), color_dist(rd));
                }
            }
        }
    }
}

void engine::Game::on_chunk_construct(entt::registry &registry, entt::entity chunk)
{
    assert(&m_entity_registry == &registry); // sanity check
    auto const &chunk_position = registry.get<engine::components::ChunkPosition>(chunk);
    m_chunks.emplace(chunk_position, chunk);
}

void engine::Game::on_chunk_destroy(entt::registry &registry, entt::entity chunk)
{
    assert(&m_entity_registry == &registry); // sanity check
    auto const &chunk_position = registry.get<engine::components::ChunkPosition>(chunk);
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