#ifndef ENGINE_CHUNK_MESH_GENERATION_HPP
#define ENGINE_CHUNK_MESH_GENERATION_HPP

#include "engine/rendering/block.hpp"

#include <cstdint>
#include <vector>

namespace engine {
    struct chunk_t;

    std::vector<rendering::block_vertex_t> generate_solid_mesh(chunk_t const&);
    std::vector<rendering::block_vertex_t> generate_translucent_mesh(chunk_t const&);

} // namespace engine

#endif
