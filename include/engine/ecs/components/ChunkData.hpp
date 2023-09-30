#pragma once

#include <engine/Block.hpp>
#include <engine/serializable_component.hpp>

namespace engine::components {

    struct ChunkData {
        constexpr static std::size_t chunk_size = 16u;

        engine::Block blocks[chunk_size * chunk_size * chunk_size];
    };

} // namespace engine::components

SERIALIZABLE_COMPONENT(engine::components::ChunkData, blocks)
