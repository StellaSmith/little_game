#include <engine/Camera.hpp>
#include <engine/Game.hpp>

#include <SDL_keyboard.h>
#include <SDL_scancode.h>
#include <imgui_impl_sdl2.h>

#include <algorithm>
#include <cstring>
#include <tuple>

extern engine::Camera g_camera;
static glm::vec3 previous_camera_position {};

int g_render_distance_horizontal = 12;
int g_render_distance_vertical = 4;
float g_mouse_sensitivity = 1;

void engine::Game::update([[maybe_unused]] engine::Game::clock_type::duration delta)
{

    const double d_delta = std::chrono::duration<double>(delta).count();
    auto const *const keyboard_state = SDL_GetKeyboardState(nullptr);

    constexpr float camera_speed = 25.0f;

    const glm::vec3 right = -glm::cross(g_camera.up, g_camera.forward);
    if (keyboard_state[SDL_SCANCODE_W])
        g_camera.position += glm::normalize(glm::vec3 { -g_camera.forward.x, 0.0f, g_camera.forward.z }) * camera_speed * static_cast<float>(d_delta);
    if (keyboard_state[SDL_SCANCODE_S])
        g_camera.position -= glm::normalize(glm::vec3 { -g_camera.forward.x, 0.0f, g_camera.forward.z }) * camera_speed * static_cast<float>(d_delta);
    if (keyboard_state[SDL_SCANCODE_D])
        g_camera.position += glm::vec3 { -right.x, 0.0f, right.z } * camera_speed * static_cast<float>(d_delta);
    if (keyboard_state[SDL_SCANCODE_A])
        g_camera.position -= glm::vec3 { -right.x, 0.0f, right.z } * camera_speed * static_cast<float>(d_delta);
    if (keyboard_state[SDL_SCANCODE_SPACE])
        g_camera.position.y += camera_speed * static_cast<float>(d_delta);
    if (keyboard_state[SDL_SCANCODE_LSHIFT])
        g_camera.position.y -= camera_speed * static_cast<float>(d_delta);

    m_renderer->imgui_new_frame(window());

    if (ImGui::Begin("Camera")) {
        ImGui::SliderFloat("FOV", &g_camera.fov, 30.0f, 130.0f);
        ImGui::SliderFloat("Mouse speed", &g_mouse_sensitivity, 0.1f, 10.0f);
        ImGui::SliderFloat("Near", &g_camera.near_plane, 0.01, 1000.0f);
        ImGui::SliderFloat("Far", &g_camera.far_plane, 0.01, 1000.0f);
        ImGui::Text("Position: % .5f % .5f % .5f", g_camera.position.x, g_camera.position.y, g_camera.position.z);
        ImGui::Text("Forward : % .5f % .5f % .5f", g_camera.forward.x, g_camera.forward.y, g_camera.forward.z);

        if (ImGui::Button("Reset"))
            g_camera = engine::Camera {};
    }
    ImGui::End();
    if (ImGui::Begin("Random Stuff")) {
        ImGui::Text("FPS: %.0f", 1 / d_delta);
        ImGui::SliderInt("Horizotal render distance", &g_render_distance_horizontal, 1, 20);
        ImGui::SliderInt("Vertical  render distance", &g_render_distance_vertical, 1, 20);
    }
    ImGui::End();

    previous_camera_position = g_camera.position;
}
