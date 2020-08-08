#ifndef ENGINE_RENDERING_BLOCK_HPP
#define ENGINE_RENDERING_BLOCK_HPP

#include <glm/glm.hpp>

namespace engine {
    namespace rendering {

        struct block_vertex_t {
            glm::vec3 position;
            glm::vec2 uv;
            glm::u8vec3 color = { 0xFF, 0xFF, 0xFF };
        };

    } // namespace rendering
} // namespace engine

#endif
