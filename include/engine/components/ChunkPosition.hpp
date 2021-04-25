#pragma once

#include <cstdint>
#include <glm/vec4.hpp>

namespace engine::components {

    struct ChunkPosition {
        std::int32_t x {};
        std::int32_t y {};
        std::int32_t z {};
        std::int32_t dimension {};
        constexpr operator glm::i32vec4() const noexcept
        {
            return { x, y, z, dimension };
        }
    };

} // namespace engine::components

namespace engine {
    using C_ChunkPosition = engine::components::ChunkPosition;
} // namespace engine