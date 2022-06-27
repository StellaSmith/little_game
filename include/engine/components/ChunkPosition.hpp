#pragma once

#include <cstdint>

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

namespace engine {
    using C_ChunkPosition = engine::components::ChunkPosition;
} // namespace engine

#include <boost/container_hash/hash.hpp>
#include <functional>

namespace std {

    template <>
    struct hash<engine::C_ChunkPosition> {
        std::size_t operator()(engine::C_ChunkPosition const &position) const noexcept
        {
            std::int32_t const arr[] = { position.x, position.y, position.z, position.dimension };
            return boost::hash_value(arr);
        }
    };

}