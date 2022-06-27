#ifndef ENGINE_GAME_HPP
#define ENGINE_GAME_HPP

#include <engine/BlockType.hpp>
#include <engine/assets/BlockMesh.hpp>
#include <engine/components/ChunkData.hpp>
#include <engine/components/ChunkPosition.hpp>
#include <engine/named_storage.hpp>
#include <engine/rendering/Mesh.hpp>
#include <engine/textures.hpp>
#include <utils/trees.hpp>

#include <boost/circular_buffer.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container_hash/hash.hpp>
#include <entt/entt.hpp>
#ifdef ENGINE_WITH_OPENGL
#include <glad/glad.h>
#endif
#include <sol/state.hpp>

#include <chrono>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

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

        ~Game();

    private:
        void setup_shader();
        void setup_texture();
        void setup_lua();

#ifdef ENGINE_WITH_OPENGL
        void setup_opengl();
#endif

        static int l_print(lua_State *);

        void on_chunk_construct(entt::registry &, entt::entity chunk);
        void on_chunk_destroy(entt::registry &, entt::entity chunk);

        rendering::Mesh generate_solid_mesh(engine::C_ChunkPosition const &, engine::C_ChunkData const &);
        rendering::Mesh generate_translucent_mesh(engine::C_ChunkPosition const &coord);

    public:
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
        sol::state m_lua;
        boost::circular_buffer<std::string> m_console_text;

#ifdef ENGINE_WITH_OPENGL
        GLuint m_vao;
        GLuint m_shader;

        GLuint m_projection_uniform;
        GLuint m_view_uniform;
#endif

        engine::Textures m_textures;

        entt::registry m_entity_registry;
        utils::octtree<std::int32_t, entt::entity> m_chunks;

        engine::named_storage<engine::BlockType> m_block_registry;
        entt::storage<engine::assets::BlockMesh> m_block_meshes;

        std::unordered_map<engine::C_ChunkPosition, std::pair<rendering::MeshHandle, rendering::MeshHandle>> m_chunk_meshes;
        std::unordered_map<engine::C_ChunkPosition, rendering::Mesh> m_translucent_mesh_data; // needed to sort indices when the camera moves
    };

} // namespace engine

#endif