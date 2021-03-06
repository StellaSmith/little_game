#ifndef ENGINE_GAME_HPP
#define ENGINE_GAME_HPP

#include "engine/Chunk.hpp"
#include "engine/chunk_mesh_generation.hpp"
#include "engine/rendering/Mesh.hpp"
#include "engine/textures.hpp"

#include <glad/gl.h>

#include <chrono>
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

        int get_texture_index(std::string_view) const noexcept;

        ~Game();

    private:
        void setup_shader();
        void setup_texture();

    public:
        bool running;

    private:
        GLuint m_vao;
        GLuint m_shader;

        GLuint m_projection_uniform;
        GLuint m_view_uniform;

        engine::Textures m_textures;

        std::unordered_map<glm::i32vec4, Chunk> m_chunks;
        std::unordered_map<glm::i32vec4, std::pair<rendering::MeshHandle, rendering::MeshHandle>> m_chunk_meshes;
        std::unordered_map<glm::i32vec4, rendering::Mesh> m_translucent_mesh_data; // needed to sort indices when the camera moves
    };

} // namespace engine

#endif