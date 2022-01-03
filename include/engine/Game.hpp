#ifndef ENGINE_GAME_HPP
#define ENGINE_GAME_HPP

#include <engine/BlockType.hpp>
#include <engine/NamedRegistry.hpp>
#include <engine/assets/BlockModel.hpp>
#include <engine/components/ChunkData.hpp>
#include <engine/components/ChunkPosition.hpp>
#include <engine/rendering/Mesh.hpp>
#include <engine/textures.hpp>

#include <entt/entt.hpp>
#include <glad/glad.h>
#include <sol/state.hpp>

#include <chrono>
#include <deque>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

union SDL_Event;

namespace std {
    template <auto N, typename T, auto Q>
    struct hash<glm::vec<N, T, Q>> {
        auto operator()(glm::vec<N, T, Q> const &v) const -> std::enable_if_t<std::is_trivially_copyable_v<T>, std::size_t>
        {
            return std::hash<std::string_view> {}(std::string_view { (char const *)&v.x, N * sizeof(T) });
        }
    };
} // namespace std

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

        static int l_print(lua_State *);

        void on_chunk_construct(entt::registry &, entt::entity chunk);
        void on_chunk_destroy(entt::registry &, entt::entity chunk);

        rendering::Mesh generate_solid_mesh(engine::C_ChunkPosition const &, engine::C_ChunkData const &);
        rendering::Mesh generate_translucent_mesh(glm::i32vec4 coord);

    public:
        bool running;

        engine::NamedRegistry<engine::BlockType const> &block_types() noexcept { return m_block_types; }
        engine::NamedRegistry<engine::assets::BlockModel const> &block_models() noexcept { return m_block_models; }

        engine::NamedRegistry<engine::BlockType const> const &block_types() const noexcept { return m_block_types; }
        engine::NamedRegistry<engine::assets::BlockModel const> const &block_models() const noexcept { return m_block_models; }

    private:
        sol::state m_lua;
        std::deque<std::string> m_console_text;

        GLuint m_vao;
        GLuint m_shader;

        GLuint m_projection_uniform;
        GLuint m_view_uniform;

        engine::Textures m_textures;

        entt::registry m_entity_registry;
        std::unordered_map<glm::i32vec4, entt::entity> m_chunks;

        engine::NamedRegistry<engine::BlockType const> m_block_types;
        engine::NamedRegistry<engine::assets::BlockModel const> m_block_models;

        std::unordered_map<glm::i32vec4, std::pair<rendering::MeshHandle, rendering::MeshHandle>> m_chunk_meshes;
        std::unordered_map<glm::i32vec4, rendering::Mesh> m_translucent_mesh_data; // needed to sort indices when the camera moves
    };

} // namespace engine

#endif