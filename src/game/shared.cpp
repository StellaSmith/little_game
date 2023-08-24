#include <engine/Camera.hpp>
#include <engine/Config.hpp>
#include <engine/Game.hpp>
#include <engine/components/Dirty.hpp>
#include <engine/rendering/vulkan/Renderer.hpp>
#include <math/bits.hpp>

#include <SDL_video.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>

#include <random>

engine::Camera g_camera;

void engine::Game::start()
{
    constexpr int width = 640, height = 480;
    m_renderer = std::make_unique<engine::rendering::vulkan::Renderer>(*this);
    m_window = m_renderer->create_window(
        "VGame",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    m_renderer->setup();
    m_renderer->imgui_setup();
    setup_lua();

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

void engine::Game::render()
{
    // Draw ImGui on top of the game stuff
    ImGui::Render();
    m_renderer->render(1.0f);
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
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    m_entity_registry.clear();
}

engine::Game::~Game()
{
    cleanup();
}