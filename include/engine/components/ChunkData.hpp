#pragma once

#include <engine/Block.hpp>

namespace engine::components {

    struct ChunkData {
        constexpr static std::size_t chunk_size = 16u;

        engine::Block blocks[chunk_size * chunk_size * chunk_size];
    };

} // namespace engine::components

namespace engine {
    using C_ChunkData = engine::components::ChunkData;
} // namespace engine
