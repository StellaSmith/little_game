#ifndef ENGINE_RENDERING_MESH_HPP
#define ENGINE_RENDERING_MESH_HPP

#include <engine/rendering/Vertex.hpp>

#include <cstdint>
#include <vector>

#ifdef ENGINE_WITH_OPENGL
#include <glad/glad.h>
#endif

namespace engine::rendering {
    struct Mesh {
        using vertex_type = engine::rendering::Vertex;
        using vertex_vector = std::vector<vertex_type>;
        using index_vector = std::vector<std::uint32_t>;
        vertex_vector vertices;
        index_vector indices;
    };
} // namespace engine::rendering

#endif