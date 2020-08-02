#include "engine/chunk_mesh_generation.hpp"
#include "engine/chunk_t.hpp"
#include "engine/rendering/block.hpp"
#include "math/constexpr.hpp"

#include <glad/glad.h>

#include <type_traits>
#include <vector>

template <std::size_t D, typename First, typename... Rest, typename std::enable_if_t<std::is_integral_v<std::common_type_t<First, Rest...>>, std::nullptr_t> = nullptr>
constexpr static std::size_t cube_at(First first, Rest... rest)
{
    if constexpr (sizeof...(Rest) == 0)
        return first;
    else
        return first * math::c_ipow_v<D, sizeof...(Rest)> + cube_at<D>(rest...);
}

enum Sides : std::uint8_t {
    NORTH = 1 << 0, // -z
    SOUTH = 1 << 1, // +z
    EAST = 1 << 2, // +x
    WEST = 1 << 3, // -x
    TOP = 1 << 4, // +y
    BOTTOM = 1 << 5, // -y

    NONE = 0,
    ALL = NORTH | SOUTH | EAST | WEST | TOP | BOTTOM
};

// return a flag set indicating which sides are solid
static Sides is_solid(engine::block_t const &block)
{
    switch (block.id) {
    case 0: // air
        return Sides::NONE;
    case 1: // stone
    case 2: // dirt
    case 3: // grass
        return Sides::ALL;
    default:
        return Sides::NONE; // we don't know
    }
}

using vertex = engine::rendering::block_vertex_t;
using vertex_vector = std::vector<vertex>;
using PFN_GetVertices = vertex_vector (*)(engine::block_t const &, Sides);

// most blocks are very simple, like stone, dirt, rock, etc
// might template on texture coordinates?
static vertex_vector GetVertices_Common(engine::block_t const &block, Sides sides)
{
    vertex_vector result;
    // TODO!: return the vertices of a common block based on id
    if (block.id == 1) {
        result.push_back(vertex { glm::vec3 { -0.75, -0.75, 0.0 }, glm::vec2 { 0.0, 1.0 } });
        result.push_back(vertex { glm::vec3 { +0.75, -0.75, 0.0 }, glm::vec2 { 1.0, 1.0 } });
        result.push_back(vertex { glm::vec3 { +0.00, +0.75, 0.0 }, glm::vec2 { 0.5, 0.0 } });
    }
    return result;
}

// grass uses tree textures:
//  bottom being the same as the base for dirt,
//  top begin the green one,
//  and for sides a green to dirt
static vertex_vector GetVertices_Grass(engine::block_t const &block, Sides sides)
{
    // TODO!: return the vertices of a grass block
    return {};
}

static std::vector<PFN_GetVertices> const block_vertices_table = {
    nullptr, // id 0 is air, which has no vertices
    &GetVertices_Common, // stone
    &GetVertices_Common, // dirt
    &GetVertices_Grass // grass
};

constexpr std::size_t chunk_size = engine::chunk_t::chunk_size;

vertex_vector engine::generate_solid_mesh(engine::chunk_t const &chunk)
{
    vertex_vector result;
    result.reserve(256); // avoid small allocations

    for (std::uint32_t x = 0; x < chunk_size; ++x) {
        for (std::uint32_t y = 0; y < chunk_size; ++y) {
            for (std::uint32_t z = 0; z < chunk_size; ++z) {
                engine::block_t const &block = chunk.blocks[cube_at<chunk_size>(x, y, z)];

                assert(block.id < block_vertices_table.size());

                PFN_GetVertices const pfn_GetVertices = block_vertices_table[block.id];

                if (!pfn_GetVertices) continue;
                if (!is_solid(block)) continue;

                const bool is_top_visible = y == chunk_size - 1 || !(is_solid(chunk.blocks[cube_at<chunk_size>(x, y + 1, z)]) & Sides::BOTTOM);
                const bool is_bottom_visible = y == 0 || !(is_solid(chunk.blocks[cube_at<chunk_size>(x, y - 1, z)]) & Sides::TOP);
                const bool is_east_visible = x == chunk_size - 1 || !(is_solid(chunk.blocks[cube_at<chunk_size>(x + 1, y, z)]) & Sides::WEST);
                const bool is_west_visible = x == 0 || !(is_solid(chunk.blocks[cube_at<chunk_size>(x - 1, y, z)]) & Sides::EAST);
                const bool is_north_visible = z == 0 || !(is_solid(chunk.blocks[cube_at<chunk_size>(x, y, z - 1)]) & Sides::SOUTH);
                const bool is_south_visible = z == chunk_size - 1 || !(is_solid(chunk.blocks[cube_at<chunk_size>(x, y, z + 1)]) & Sides::NORTH);

                // clang-format off
                // pack to a Sides flags
                Sides const sides = static_cast<Sides>(
                    Sides::TOP    * is_top_visible    |
                    Sides::BOTTOM * is_bottom_visible |
                    Sides::EAST   * is_east_visible   |
                    Sides::WEST   * is_west_visible   |
                    Sides::NORTH  * is_north_visible  |
                    Sides::SOUTH  * is_south_visible);
                // clang-format on

                if (!sides) continue; // no visible sides

                vertex_vector vertices = pfn_GetVertices(block, sides); // these are in block coords
                for (auto &vertex : vertices) // transform to world coords
                    vertex.position += static_cast<glm::vec3>(chunk.position) + glm::vec3 { x, y, z };
                // or maybe use std::move with an insert iterator?
                result.insert(result.end(), std::make_move_iterator(vertices.begin()), std::make_move_iterator(vertices.end()));
            }
        }
    }

    // maybe call shrink_to_fit?
    return result;
}

// will be called more frequently
vertex_vector engine::generate_translucent_mesh(engine::chunk_t const &chunk)
{
    vertex_vector result;
    result.reserve(256); // avoid small allocations

    // TODO!: Logic to generate translucent meshes
    //
    // Iterate over each block in chunk
    // If block is completely solid, skip
    // Check for visible sides similar to how it is done with the solid blocks, but if the adjacent block id is the same then interpret it as an invisible face
    // (perhaps check if the adjacent block is the same with the subid too, or do it at runtime, for cases like different glass color per subid.)
    // If block is not visible for any side, skip
    // Generate vertices

    return result;
}
