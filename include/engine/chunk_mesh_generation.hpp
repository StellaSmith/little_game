#ifndef ENGINE_CHUNK_MESH_GENERATION_HPP
#define ENGINE_CHUNK_MESH_GENERATION_HPP

#include "engine/rendering/block.hpp"

#include <cstdint>
#include <vector>

namespace engine {
    struct chunk_t;
    class Game;

    struct chunk_mesh_data_t {
        std::vector<rendering::block_vertex_t> vertices;
        std::vector<std::uint32_t> indices;
    };

    chunk_mesh_data_t generate_solid_mesh(Game const&,chunk_t const &);
    chunk_mesh_data_t generate_translucent_mesh(Game const&,chunk_t const &);

} // namespace engine

#endif
