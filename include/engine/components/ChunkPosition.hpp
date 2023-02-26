#pragma once

#include <cstdint>
#include <engine/serializable_component.hpp>

namespace engine::components {

    struct ChunkPosition {
        std::int32_t x {};
        std::int32_t y {};
        std::int32_t z {};
        std::int32_t dimension {};
    };

    constexpr bool operator==(ChunkPosition const &lhs, ChunkPosition const &rhs) noexcept
    {
        return lhs.dimension == rhs.dimension && lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }

    constexpr bool operator!=(ChunkPosition const &lhs, ChunkPosition const &rhs) noexcept
    {
        return lhs.dimension != rhs.dimension || lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
    }
} // namespace engine::components

SERIALIZABLE_COMPONENT(engine::components::ChunkPosition, x, y, z, dimension)

#include <boost/container_hash/hash.hpp>

namespace std {

    template <>
    struct hash<engine::components::ChunkPosition> {
        std::size_t operator()(engine::components::ChunkPosition const &position) const noexcept
        {
            std::int32_t const arr[] = { position.x, position.y, position.z, position.dimension };
            return boost::hash_value(arr);
        }
    };

}
