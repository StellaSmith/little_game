#ifndef ENGINE_RENDERING_BLOCK_HPP
#define ENGINE_RENDERING_BLOCK_HPP

#include <glm/glm.hpp>

namespace engine {
    namespace rendering {
        struct Vertex {
            glm::vec3 position {};
            glm::vec2 uv {};
            glm::vec3 color { 0xFF, 0xFF, 0xFF };
            glm::vec3 light {};
            glm::uvec2 textures {};
        };
    } // namespace rendering
} // namespace engine

#endif
