#ifndef ENGINE_RENDERING_MESH_HPP
#define ENGINE_RENDERING_MESH_HPP

#include <engine/rendering/Vertex.hpp>

#include <cstdint>
#include <vector>

namespace engine::rendering {
    struct Mesh {
        using vertex_type = engine::rendering::Vertex;
        using vertex_vector = std::vector<vertex_type>;
        using index_vector = std::vector<std::uint32_t>;
        vertex_vector vertices;
        index_vector indices;
    };

    // OpenGL handle to a mesh
    struct MeshHandle {
        std::uint32_t vertex_buffer;
        std::uint32_t index_buffer;
        std::size_t index_count;
    };
} // namespace engine::rendering

#endif