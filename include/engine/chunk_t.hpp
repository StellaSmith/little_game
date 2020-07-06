#ifndef ENGINE_CHUNK_HPP
#define ENGINE_CHUNK_HPP

#include "operators.hpp"

#include <cstddef>
#include <array>
#include <glm/glm.hpp>

namespace engine
{
    struct block_t
    {
        std::uint32_t id = 0;
        std::uint32_t subid = 0;

        union data_t {
            std::uint64_t u64;
            double f64;
            void *ptr;
        } data{.ptr = nullptr};

        struct Vertex
        {
            glm::vec3 position;
            glm::vec2 uv;
        };
    };

    struct chunk_t
    {
        constexpr static auto chunk_size = 16_sz;

        glm::u32vec4 position;
        bool modified;
        std::array<block_t, chunk_size * chunk_size * chunk_size> blocks;
    };

} // namespace engine

#endif