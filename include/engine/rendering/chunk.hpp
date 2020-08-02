#ifndef ENGINE_RENDERING_CHUNK_HPP
#define ENGINE_RENDERING_CHUNK_HPP

#include <glad/glad.h>

namespace engine {
    namespace rendering {

        struct chunk_meshes{
            GLuint solid_vertex_buffer, translucent_vertex_buffer;
            std::size_t solid_vertex_count, translucent_vertex_count;
        };

    } // namespace rendering
} // namespace engine

#endif
