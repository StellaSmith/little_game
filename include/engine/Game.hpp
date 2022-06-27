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
    struct VectorHasher {
        template <glm::length_t L, typename T, glm::qualifier Q>
        std::size_t operator()(glm::vec<L, T, Q> const &vec) const noexcept
        {
            return boost::hash_value([&]<std::size_t... I>(std::index_sequence<I...>) {
                return std::make_tuple(vec[I]...);
            }(std::make_index_sequence<L> {}));
        }
    };

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
        rendering::Mesh generate_translucent_mesh(glm::i32vec4 coord);

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

        auto &block_models() noexcept { return m_block_models; }
        auto const &block_models() const noexcept { return m_block_models; }

        auto &block_model_names() noexcept { return m_block_model_names; }
        auto const &block_model_names() const noexcept { return m_block_model_names; }

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
        entt::basic_storage<entt::id_type, engine::assets::BlockMesh> m_block_meshes;

        boost::container::flat_map<std::string, entt::id_type, std::less<>> m_block_model_names;
        entt::basic_storage<entt::id_type, engine::assets::BlockMesh> m_block_models;

        std::unordered_map<glm::i32vec4, std::pair<rendering::MeshHandle, rendering::MeshHandle>, VectorHasher> m_chunk_meshes;
        std::unordered_map<glm::i32vec4, rendering::Mesh, VectorHasher> m_translucent_mesh_data; // needed to sort indices when the camera moves
    };

} // namespace engine

#endif