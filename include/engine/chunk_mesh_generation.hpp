#ifndef ENGINE_CHUNK_MESH_GENERATION_HPP
#define ENGINE_CHUNK_MESH_GENERATION_HPP

#include "engine/rendering/Mesh.hpp"

#include <cstdint>
#include <vector>

namespace engine {
    struct Chunk;
    class Game;

    rendering::Mesh generate_solid_mesh(Chunk const &);
    rendering::Mesh generate_translucent_mesh(Chunk const &);

} // namespace engine

#endif
