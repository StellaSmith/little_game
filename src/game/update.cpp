#include "engine/game.hpp"
#include "engine/camera.hpp"
#include "math/bits.hpp"

#include <imgui.h>
#include <glad/glad.h>

#include <algorithm>

extern engine::Camera g_camera;
static glm::vec3 previous_camera_position {};

static std::vector<std::uint32_t> get_sorted_indices(std::vector<engine::rendering::block_vertex_t> const &vertices, std::vector<std::uint32_t> indices)
{
    using face_t = std::array<std::uint32_t, 3>;
    auto begin = reinterpret_cast<face_t *>(indices.data());
    auto end = reinterpret_cast<face_t *>(indices.data() + indices.size());

    std::sort(begin, end, [&vertices](face_t const &lhs, face_t const &rhs) {
        auto const lhs_center = (vertices[lhs[0]].position + vertices[lhs[1]].position + vertices[lhs[2]].position) / 3.0f;
        auto const rhs_center = (vertices[rhs[0]].position + vertices[rhs[1]].position + vertices[rhs[2]].position) / 3.0f;
        auto const lhs_distance = glm::length(lhs_center - g_camera.position);
        auto const rhs_distance = glm::length(rhs_center - g_camera.position);
        return lhs_distance < rhs_distance;
    });

    return indices;
}

void engine::Game::update([[maybe_unused]] engine::Game::clock_type::duration delta)
{
    if (ImGui::Begin("Camera Controls")) {
        ImGui::SliderFloat("FOV", &g_camera.fov, 30.0f, 130.0f);
        ImGui::SliderFloat("Near", &g_camera.near, 0.01, 1000.0f);
        ImGui::SliderFloat("Far", &g_camera.far, 0.01, 1000.0f);

        if (ImGui::Button("Reset"))
            g_camera = engine::Camera {};
    }
    ImGui::End();
    if (ImGui::Begin("Random Stuff")) {
        static float col[3] { 0, 0, 0 };
        if (ImGui::ColorPicker3("Colorful block color", col)) {
            chunk_t &chunk = m_chunks.find(glm::i32vec4 { 0, 0, 0, 0 })->second;
            chunk.modified = true;
            chunk.blocks[0].data.u64 = math::pack_u32(col[0] * 255.0f, col[1] * 255.0f, col[2] * 255.0f);
            ImGui::Text("Chunk Modified");
        }
    }
    ImGui::End();

    std::vector<glm::i32vec4> to_delete;
    std::vector<chunk_t> to_add;
    for (auto &[k, chunk] : m_chunks) {
        if (chunk.modified) {
            auto it = std::find_if(m_chunk_meshes.begin(), m_chunk_meshes.end(), [&pos = chunk.position](auto const &p) { return p.first == pos; });
            //auto it = m_chunk_meshes.find(chunk.position);
            if (it == m_chunk_meshes.end()) {
                GLuint buffers[4];
                glGenBuffers(std::size(buffers), buffers);

                m_chunk_meshes.emplace_back(chunk.position, rendering::chunk_meshes { buffers[0], buffers[1], buffers[2], buffers[3], 0, 0 });
                it = m_chunk_meshes.end() - 1;
            }

            auto const solid_mesh = generate_solid_mesh(*this, chunk);
            auto const translucent_mesh = generate_translucent_mesh(*this, chunk);

            auto const sorted_indices = get_sorted_indices(translucent_mesh.vertices, translucent_mesh.indices);

            // TODO: Vertex deduplication

            glBindBuffer(GL_ARRAY_BUFFER, it->second.solid_vertex_buffer);
            glBufferData(GL_ARRAY_BUFFER, solid_mesh.vertices.size() * sizeof(*solid_mesh.vertices.data()), solid_mesh.vertices.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->second.solid_index_buffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, solid_mesh.indices.size() * sizeof(*solid_mesh.indices.data()), solid_mesh.indices.data(), GL_DYNAMIC_DRAW);
            it->second.solid_index_count = solid_mesh.indices.size();

            glBindBuffer(GL_ARRAY_BUFFER, it->second.translucent_vertex_buffer);
            glBufferData(GL_ARRAY_BUFFER, translucent_mesh.vertices.size() * sizeof(*translucent_mesh.vertices.data()), translucent_mesh.vertices.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->second.translucent_index_buffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sorted_indices.size() * sizeof(*sorted_indices.data()), sorted_indices.data(), GL_STREAM_DRAW);
            it->second.translucent_index_count = sorted_indices.size();

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            if (auto mesh_data = m_translucent_mesh_data.find(chunk.position); mesh_data != m_translucent_mesh_data.end())
                mesh_data->second = std::move(translucent_mesh);
            else
                m_translucent_mesh_data.emplace(chunk.position, std::move(translucent_mesh));

            chunk.modified = false;
        } else if (g_camera.position != previous_camera_position) {
            auto p_mesh = std::find_if(m_chunk_meshes.begin(), m_chunk_meshes.end(), [&position = chunk.position](auto const &p) { return p.first == position; });
            auto p_data = m_translucent_mesh_data.find(chunk.position);
            if (p_mesh != m_chunk_meshes.end() && p_data != m_translucent_mesh_data.end()) {
                auto const sorted_indices = get_sorted_indices(p_data->second.vertices, p_data->second.indices);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p_mesh->second.translucent_index_buffer);
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
