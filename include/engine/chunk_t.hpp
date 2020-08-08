#ifndef ENGINE_CHUNK_HPP
#define ENGINE_CHUNK_HPP

#include "engine/block_t.hpp"
#include "operators.hpp"

#include <array>
#include <cstddef>

#include <glm/glm.hpp>

namespace engine {

    struct chunk_t {
        constexpr static auto chunk_size = 16_sz;

        glm::i32vec4 position; // x, y, z, d. Where d is the dimension id (0 = overworld)
        bool modified; // needs to regenerate all the meshes again
        std::array<block_t, chunk_size * chunk_size * chunk_size> blocks;
    };

} // namespace engine

#endif
