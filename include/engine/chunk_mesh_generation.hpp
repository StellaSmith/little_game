#ifndef ENGINE_CHUNK_MESH_GENERATION_HPP
#define ENGINE_CHUNK_MESH_GENERATION_HPP

#include "engine/rendering/Mesh.hpp"

#include <cstdint>
#include <vector>

namespace engine {
    struct Chunk;
    class Game;

    rendering::Mesh generate_solid_mesh(Game const &, Chunk const &);
    rendering::Mesh generate_translucent_mesh(Game const &, Chunk const &);

} // namespace engine

#endif
