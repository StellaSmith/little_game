#include <engine/ecs/components/Camera.hpp>
#include <engine/ecs/components/LocalPlayer.hpp>
#include <engine/ecs/systems/localplayer_camera.hpp>
#include <engine/rendering/IRenderer.hpp>

#include <SDL_keyboard.h>
#include <entt/entt.hpp>

constexpr float camera_speed = 25.0f;

void engine::systems::localplayer_camera(entt::registry &registry, std::chrono::duration<double> delta)
{
    auto const keyboard_state = SDL_GetKeyboardState(nullptr);
    auto &renderer = registry.ctx().get<std::unique_ptr<engine::rendering::IRenderer>>();
    (void)renderer;

    auto view = registry.view<engine::components::LocalPlayer, engine::components::Camera>();
    view.each([&](engine::components::Camera &camera) {
        const glm::vec3 right = -glm::cross(camera.up, camera.forward);
        if (keyboard_state[SDL_SCANCODE_W])
            camera.position += glm::normalize(glm::vec3 { -camera.forward.x, 0.0f, camera.forward.z }) * camera_speed * static_cast<float>(delta.count());
        if (keyboard_state[SDL_SCANCODE_S])
            camera.position -= glm::normalize(glm::vec3 { -camera.forward.x, 0.0f, camera.forward.z }) * camera_speed * static_cast<float>(delta.count());
        if (keyboard_state[SDL_SCANCODE_D])
            camera.position += glm::vec3 { -right.x, 0.0f, right.z } * camera_speed * static_cast<float>(delta.count());
        if (keyboard_state[SDL_SCANCODE_A])
            camera.position -= glm::vec3 { -right.x, 0.0f, right.z } * camera_speed * static_cast<float>(delta.count());
        if (keyboard_state[SDL_SCANCODE_SPACE])
            camera.position.y += camera_speed * static_cast<float>(delta.count());
        if (keyboard_state[SDL_SCANCODE_LSHIFT])
            camera.position.y -= camera_speed * static_cast<float>(delta.count());
    });
}