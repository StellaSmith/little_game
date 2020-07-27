#ifndef ENGINE_CHUNK_RENDERER_HPP
#define ENGINE_CHUNK_RENDERER_HPP

#include <cstdint>

namespace engine {
    struct chunk_t;

    struct chunk_renderer {
        struct chunk_meshes {
            unsigned int solid_buffer; // blocks whose textures have fully opaque or fully transparent pixels (alpha == 0.0 or alpha == 1.0)
            unsigned int translucent_buffer; // blocks whose textures that have some transparency (alpha != 0.0 and alpha != 1.0)

            std::uint32_t solid_vertices;
            std::uint32_t translucent_vertices;
        };

        // Filles the buffers with vertices
        // Returns 0 on success
        // Returns -1 on error
        static int generate_mesh(chunk_t const &chunk, chunk_meshes &buffer) noexcept;
    };

} // namespace engine

#endif