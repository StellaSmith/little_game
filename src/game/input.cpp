#include <engine/Camera.hpp>
#include <engine/Game.hpp>

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <algorithm>
#include <cmath>

extern engine::Camera g_camera;
extern float g_mouse_sensitivity;
static glm::vec2 mouse_rot { 0, 0 };

void engine::Game::input(SDL_Event const &event)
{
    if (event.type == SDL_QUIT)
        stop();
    else if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
#ifdef ENGINE_WITH_OPENGL
            glViewport(0, 0, event.window.data1, event.window.data2);
#endif
        }
    } else if (event.type == SDL_MOUSEMOTION) {
        float const speed_multipler = g_mouse_sensitivity * -0.01f;

        mouse_rot += glm::vec2 { event.motion.xrel, event.motion.yrel } * speed_multipler;
        mouse_rot.y = std::clamp(mouse_rot.y, -glm::pi<float>() / 2.0f, glm::pi<float>() / 2.0f);

        g_camera.forward = glm::normalize(glm::rotate(glm::vec3 { 0.0f, 0.0f, -1.0f }, mouse_rot.x, glm::vec3 { 0.0f, 1.0f, 0.0f }));
        auto kinda_right = glm::normalize(glm::cross(glm::vec3 { g_camera.forward.x, 0.0f, g_camera.forward.z }, glm::vec3 { 0.0f, 1.0f, 0.0f }));
        g_camera.forward = glm::normalize(glm::rotate(g_camera.forward, mouse_rot.y, kinda_right));
        g_camera.up = glm::normalize(glm::rotate(glm::vec3 { 0.0f, 1.0f, 0.0f }, mouse_rot.y, kinda_right));
    } else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_ESCAPE)
            SDL_SetRelativeMouseMode(SDL_GetRelativeMouseMode() == SDL_TRUE ? SDL_FALSE : SDL_TRUE);
    }
}