#ifndef ENGINE_GAME_HPP
#define ENGINE_GAME_HPP

#include <engine/BlockType.hpp>
#include <engine/assets/BlockMesh.hpp>
#include <engine/components/ChunkData.hpp>
#include <engine/components/ChunkPosition.hpp>
#include <engine/named_storage.hpp>
#include <engine/rendering/IRenderer.hpp>
#include <engine/rendering/Mesh.hpp>
#include <engine/sdl/Window.hpp>

#include <boost/circular_buffer.hpp>
#include <entt/entt.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

union SDL_Event;

namespace engine {

    class Game {
    public:
        using clock_type = std::conditional_t<std::chrono::high_resolution_clock::is_steady, std::chrono::high_resolution_clock, std::chrono::steady_clock>;

        void start();
        void stop();
        void update(clock_type::duration);
        void render();
        void input(SDL_Event const &);
        void cleanup();

        int get_texture_index(std::string_view) const noexcept;

        entt::registry &registry() noexcept
        {
            return m_entity_registry;
        }

        engine::sdl::Window &window() noexcept
        {
            return m_window;
        }

        ~Game();

    private:
        void setup_renderer();

        void on_chunk_construct(entt::registry &, entt::entity chunk);
        void on_chunk_destroy(entt::registry &, entt::entity chunk);

    public:
        rendering::Mesh generate_solid_mesh(engine::components::ChunkPosition const &, engine::components::ChunkData const &);
        rendering::Mesh generate_translucent_mesh(engine::components::ChunkPosition const &coord);

        bool running;

        auto &block_registry() noexcept
        {
            return m_block_registry;
        }

        auto const &block_registry() const noexcept
        {
            return m_block_registry;
        }

        auto const &block_meshes() const noexcept
        {
            return m_block_meshes;
        }

    private:
        std::unique_ptr<rendering::IRenderer> m_renderer = nullptr;
        engine::sdl::Window m_window = nullptr;

        entt::registry m_entity_registry;
        std::unordered_map<engine::components::ChunkPosition, entt::entity> m_chunks;
        // utils::octtree<std::int32_t, entt::entity> m_chunks;

        engine::named_storage<engine::BlockType> m_block_registry;
        entt::storage<engine::assets::BlockMesh> m_block_meshes;
    };

} // namespace engine

#endif