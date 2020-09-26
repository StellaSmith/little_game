#ifndef ENGINE_RENDERING_BLOCK_HPP
#define ENGINE_RENDERING_BLOCK_HPP

#include <glm/glm.hpp>

namespace engine {
    namespace rendering {

        struct block_vertex_t {
            glm::vec3 position;
            glm::vec3 uv;
            glm::u8vec3 color = { 0xFF, 0xFF, 0xFF };
            glm::u8vec3 light = {};
        };

    } // namespace rendering
} // namespace engine

#endif
