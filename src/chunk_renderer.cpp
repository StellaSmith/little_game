#include "engine/chunk_renderer.hpp"
#include "engine/chunk_t.hpp"
#include "math/constexpr.hpp"

#include <glad/glad.h>

#include <vector>
#include <type_traits>

template <std::size_t D, typename First, typename... Rest, typename std::enable_if_t<std::is_integral_v<std::common_type_t<First, Rest...>>, std::nullptr_t> = nullptr>
constexpr static std::size_t cube_at(First first, Rest... rest)
{
    if constexpr (sizeof...(Rest) == 0)
        return first;
    else
        return first * math::c_ipow_v<D, sizeof...(Rest)> + cube_at<D>(rest...);
}

using vertex_vector = std::vector<engine::block_t::Vertex>;

static void add_vertices_stone(vertex_vector &solid_vertices, vertex_vector &translucent_vertices, glm::vec3 position, engine::block_t const &block)
{
    solid_vertices.insert(
        solid_vertices.end(),
        std::initializer_list<engine::block_t::Vertex>{
            {position + glm::vec3{+0.0, +0.5, +0.0}, glm::vec2{0.5, 0.0}},
            {position + glm::vec3{-0.5, -0.5, +0.0}, glm::vec2{0.0, 1.0}},
            {position + glm::vec3{+0.5, -0.5, +0.0}, glm::vec2{1.0, 1.0}}});
}

using PFN_GetVertices = void (*)(vertex_vector &solid_vertices, vertex_vector &translucent_vertices, glm::vec3 position, engine::block_t const &block);
static auto vertex_func_table = []() -> std::vector<PFN_GetVertices> {
    return {
        nullptr,
        &add_vertices_stone};
}();

int engine::chunk_renderer::generate_mesh(engine::chunk_t const &chunk, engine::chunk_renderer::chunk_meshes &meshes) noexcept
{
    vertex_vector solid_vertices, translucent_vertices;
    solid_vertices.reserve(128_sz);
    translucent_vertices.reserve(128_sz);

    for (std::uint32_t x = 0; x < engine::chunk_t::chunk_size; ++x)
        for (std::uint32_t y = 0; y < engine::chunk_t::chunk_size; ++y)
            for (std::uint32_t z = 0; z < engine::chunk_t::chunk_size; ++z)
            {
                auto const flat_coord = cube_at<engine::chunk_t::chunk_size>(x, y, z);
                auto const &block = chunk.blocks[flat_coord];
                if (block.id < vertex_func_table.size() && vertex_func_table[block.id])
                    vertex_func_table[block.id](
                        solid_vertices,
                        translucent_vertices,
                        glm::vec3{x + chunk.position.x * engine::chunk_t::chunk_size,
                                  y + chunk.position.y * engine::chunk_t::chunk_size,
                                  z + chunk.position.z * engine::chunk_t::chunk_size},
                        block);
            }

    glBindBuffer(GL_ARRAY_BUFFER, meshes.solid_buffer);
    glBufferData(GL_ARRAY_BUFFER, solid_vertices.size() * sizeof(vertex_vector::value_type), solid_vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, meshes.translucent_buffer);
    glBufferData(GL_ARRAY_BUFFER, translucent_vertices.size() * sizeof(vertex_vector::value_type), translucent_vertices.data(), GL_DYNAMIC_DRAW);

    meshes.solid_vertices = solid_vertices.size();
    meshes.translucent_vertices = translucent_vertices.size();

    return 0;
}