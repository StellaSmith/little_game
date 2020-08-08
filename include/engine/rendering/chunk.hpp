#ifndef ENGINE_RENDERING_CHUNK_HPP
#define ENGINE_RENDERING_CHUNK_HPP

#include <glad/glad.h>

namespace engine {
    namespace rendering {

        struct chunk_meshes {
            GLuint solid_vertex_buffer, solid_index_buffer, translucent_vertex_buffer, translucent_index_buffer;
            std::size_t solid_index_count, translucent_index_count;
        };

    } // namespace rendering
} // namespace engine

#endif
