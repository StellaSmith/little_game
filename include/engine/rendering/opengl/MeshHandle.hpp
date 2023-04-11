#ifndef ENGINE_WITH_OPENGL
#error "Engine is configured to not use OpenGL but this file was included"
#endif

#ifndef ENGINE_RENDERING_OPENGL_MESH_HANDLE_HPP
#define ENGINE_RENDERING_OPENGL_MESH_HANDLE_HPP

#include <glad/glad.h>

#include <cstdint>

namespace engine::rendering::opengl {
    struct MeshHandle {
        GLuint vertex_buffer;
        GLuint index_buffer;
        std::uint32_t index_count;
    };
}

#endif