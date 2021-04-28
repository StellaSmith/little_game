#pragma once

#include <glm/vec3.hpp>

namespace engine {
    struct Camera {
        glm::vec3 position = glm::vec3 { -8.0f, 0.0f, -8.0f };
        glm::vec3 forward = glm::vec3 { 0.0f, 0.0f, 1.0f };
        glm::vec3 up = glm::vec3 { 0.0f, 1.0f, 0.0f };
        float fov = 60.0f;
        float near_plane = 0.1f;
        float far_plane = 100.0f;
    };
} // namespace engine
