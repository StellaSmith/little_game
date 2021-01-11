#include "engine/camera.hpp"
#include "engine/game.hpp"
#include "engine/rendering/Mesh.hpp"
#include "math/bits.hpp"

#include <glad/glad.h>
#include <imgui.h>

#include <algorithm>

extern engine::Camera g_camera;
static glm::vec3 previous_camera_position {};

static auto get_sorted_indices(engine::rendering::Mesh const &mesh)
{
    auto result = mesh.indices;
    using face_t = std::array<std::uint32_t, 3>;
    auto begin = reinterpret_cast<face_t *>(result.data());
    auto end = reinterpret_cast<face_t *>(result.data() + result.size());

    std::sort(begin, end, [&vertices = mesh.vertices](face_t const &lhs, face_t const &rhs) {
        auto const lhs_center = (vertices[lhs[0]].position + vertices[lhs[1]].position + vertices[lhs[2]].position) / 3.0f;
        auto const rhs_center = (vertices[rhs[0]].position + vertices[rhs[1]].position + vertices[rhs[2]].position) / 3.0f;
        auto const lhs_distance = glm::length(lhs_center - g_camera.position);
        auto const rhs_distance = glm::length(rhs_center - g_camera.position);
        return lhs_distance < rhs_distance;
    });

    return result;
}
#include <SDL.h>

int g_render_distance_horizontal = 12;
int g_render_distance_vertical = 4;

#include <cstring>
#include <tuple>

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

    if (ImGui::Begin("Camera")) {
        ImGui::SliderFloat("FOV", &g_camera.fov, 30.0f, 130.0f);
        ImGui::SliderFloat("Near", &g_camera.near, 0.01, 1000.0f);
        ImGui::SliderFloat("Far", &g_camera.far, 0.01, 1000.0f);
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

    if (ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        static char line_buf[1024 * 8] {};
        static bool enter = false;

        if (std::exchange(enter, false))
            ImGui::SetNextWindowFocus();

        enter |= ImGui::InputText("##Input", line_buf, std::size(line_buf), ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::SameLine();
        enter |= ImGui::Button("Eval");

        ImGui::SameLine();
        if (ImGui::Button("Clear"))
            m_console_text.clear();

        if (enter) {
            m_console_text.push_back(line_buf);
            std::memset(line_buf, 0, sizeof(line_buf));
        }

        ImGui::BeginChild("Output", {}, true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
        static float prev_scroll = 0;
        static float prev_max_scroll = 0;

        if (prev_scroll == prev_max_scroll)
            ImGui::SetScrollY(ImGui::GetScrollMaxY());

        for (std::string const &line : m_console_text) {
            ImGui::TextUnformatted(line.data(), line.data() + line.size());
        }

        prev_scroll = ImGui::GetScrollY();
        prev_max_scroll = ImGui::GetScrollMaxY();

        ImGui::EndChild();
    }
    ImGui::End();

    std::vector<glm::i32vec4> to_delete;
    std::vector<Chunk> to_add;

    for (auto &[k, chunk] : m_chunks) {
        if (chunk.modified) {
            auto it = m_chunk_meshes.find(chunk.position);
            if (it == m_chunk_meshes.end()) {
                GLuint buffers[4];
                glGenBuffers(std::size(buffers), buffers);
                std::tie(it, std::ignore) = m_chunk_meshes.emplace(chunk.position, std::make_pair(rendering::MeshHandle { buffers[0], buffers[1], 0 }, rendering::MeshHandle { buffers[2], buffers[3], 0 }));
            }

            auto const solid_mesh = generate_solid_mesh(chunk);
            auto const translucent_mesh = generate_translucent_mesh(chunk);

            auto const sorted_indices = get_sorted_indices(translucent_mesh);

            // TODO: Vertex deduplication

            glBindBuffer(GL_ARRAY_BUFFER, it->second.first.vertex_buffer);
            glBufferData(GL_ARRAY_BUFFER, solid_mesh.vertices.size() * sizeof(*solid_mesh.vertices.data()), solid_mesh.vertices.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->second.first.index_buffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, solid_mesh.indices.size() * sizeof(*solid_mesh.indices.data()), solid_mesh.indices.data(), GL_DYNAMIC_DRAW);
            it->second.first.index_count = solid_mesh.indices.size();

            glBindBuffer(GL_ARRAY_BUFFER, it->second.second.vertex_buffer);
            glBufferData(GL_ARRAY_BUFFER, translucent_mesh.vertices.size() * sizeof(*translucent_mesh.vertices.data()), translucent_mesh.vertices.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->second.second.index_buffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sorted_indices.size() * sizeof(*sorted_indices.data()), sorted_indices.data(), GL_STREAM_DRAW);
            it->second.second.index_count = sorted_indices.size();

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            if (auto mesh_data = m_translucent_mesh_data.find(chunk.position); mesh_data != m_translucent_mesh_data.end())
                mesh_data->second = std::move(translucent_mesh);
            else
                m_translucent_mesh_data.emplace(chunk.position, std::move(translucent_mesh));

            chunk.modified = false;
        } else if (g_camera.position != previous_camera_position) {
            auto p_mesh = m_chunk_meshes.find(chunk.position);
            auto p_data = m_translucent_mesh_data.find(chunk.position);
            if (p_mesh != m_chunk_meshes.end() && p_data != m_translucent_mesh_data.end()) {
                auto const sorted_indices = get_sorted_indices(p_data->second);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p_mesh->second.second.index_buffer);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sorted_indices.size() * sizeof(*sorted_indices.data()), sorted_indices.data(), GL_STREAM_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }
        }
    }

    for (auto position : to_delete) {
        m_chunks.erase(position);
        m_translucent_mesh_data.erase(position);
        auto p_mesh = std::find_if(m_chunk_meshes.begin(), m_chunk_meshes.end(), [position](auto const &p) { return p.first == position; });
        if (p_mesh != m_chunk_meshes.end())
            m_chunk_meshes.erase(p_mesh);
    }

    for (auto &chunk : to_add) {
        chunk.modified = true; // will generate the meshes next iteration
        m_chunks.emplace(chunk.position, std::move(chunk));
    }

    previous_camera_position = g_camera.position;
}
