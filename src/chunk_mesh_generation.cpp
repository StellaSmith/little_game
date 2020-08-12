#include "engine/chunk_mesh_generation.hpp"
#include "engine/chunk_t.hpp"
#include "engine/rendering/block.hpp"
#include "math/constexpr.hpp"

#include <glad/glad.h>

#include <algorithm>
#include <cstring>
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
using index_vector = std::vector<std::uint32_t>;
using PFN_GetVertices = engine::chunk_mesh_data_t (*)(engine::block_t const &, Sides);

// most blocks are very simple, like stone, dirt, rock, etc
// might template on texture coordinates?
static engine::chunk_mesh_data_t GetVertices_Common(engine::block_t const &block, Sides sides)
{
    engine::chunk_mesh_data_t result;
    // TODO!: return the vertices of a common block based on id
    if (block.id == 2) {
        result.vertices = {
            vertex { { -0.5, -0.5, 0.0 }, { 0.0, 1.0 } },
            vertex { { +0.5, -0.5, 0.0 }, { 1.0, 1.0 } },
            vertex { { +0.0, +0.5, 0.0 }, { 0.5, 0.0 } }
        };

        result.indices = { 2u, 1u, 0u };
    }
    return result;
}
#include "math/bits.hpp"
static engine::chunk_mesh_data_t GetVertices_Colorful(engine::block_t const &block, Sides sides)
{
    glm::u8vec4 color = math::unpack_u32(static_cast<std::uint32_t>(block.data.u64));
    engine::chunk_mesh_data_t result;
    result.vertices = {
        // TODO: Fix uv coords
        vertex { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f }, color },
        vertex { { +0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, color },
        vertex { { +0.5f, +0.5f, +0.5f }, { 0.0f, 0.0f }, color },
        vertex { { +0.5f, -0.5f, +0.5f }, { 0.0f, 0.0f }, color },
        vertex { { -0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f }, color },
        vertex { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, color },
        vertex { { -0.5f, +0.5f, +0.5f }, { 0.0f, 0.0f }, color },
        vertex { { -0.5f, -0.5f, +0.5f }, { 0.0f, 0.0f }, color }
    };
    result.indices = {
        // clang-format off
        0, 4, 2,
        3, 6, 7,
        7, 6, 4,
        5, 1, 3,
        1, 2, 3,
        5, 4, 0,
        6, 3, 2,
        2, 1, 0,
        0, 1, 5, 
        4, 5, 7, 
        3, 7, 5,
        2, 4, 6
        // clang-format on
    };
    return result;
}

// grass uses tree textures:
//  bottom being the same as the base for dirt,
//  top begin the green one,
//  and for sides a green to dirt
static engine::chunk_mesh_data_t GetVertices_Grass(engine::block_t const &block, Sides sides)
{
    // TODO!: return the vertices of a grass block
    return {};
}

static std::vector<PFN_GetVertices> const block_vertices_table = {
    nullptr, // id 0 is air, which has no vertices
    &GetVertices_Colorful,
    &GetVertices_Common, // stone
    &GetVertices_Common, // dirt
    &GetVertices_Grass // grass
};

static glm::u8vec4 get_produced_light(engine::block_t const &block) // r, g, b, intensity
{
    if (block.id == 1) {
        constexpr float default_intensity = 16.0f;
        auto color = math::unpack_u32(static_cast<std::uint32_t>(block.data.u64));
        float const intensity = glm::length(glm::vec3 { color.x, color.y, color.z } / 255.0f);
        color.w = static_cast<std::uint8_t>(intensity * default_intensity);
        return color;
    }
    return { 0, 0, 0, 0 };
}

static std::size_t remove_duplicate_vertices(engine::chunk_mesh_data_t &chunk_data)
{
    std::size_t erased = 0;
    for (std::uint32_t index : chunk_data.indices) {
        auto const &to_find = chunk_data.vertices[index];
        auto it = std::find_if(chunk_data.vertices.begin() + index + 1, chunk_data.vertices.end(), [&to_find](auto const &vertex) {
            return std::memcmp(&to_find, &vertex, sizeof(to_find)) == 0;
        });
        if (it == chunk_data.vertices.end()) continue;
        auto found_index = static_cast<std::uint32_t>(it - chunk_data.vertices.begin());
        std::replace(chunk_data.indices.begin(), chunk_data.indices.end(), found_index, index);
        chunk_data.vertices.erase(it);
        ++erased;
    }
    return erased;
}

static std::size_t calculate_light(engine::chunk_mesh_data_t &chunk_data) { return 0; }

constexpr std::size_t chunk_size = engine::chunk_t::chunk_size;

engine::chunk_mesh_data_t engine::generate_solid_mesh(engine::chunk_t const &chunk)
{
    engine::chunk_mesh_data_t result;

    // avoid small allocations
    result.vertices.reserve(256);
    result.indices.reserve(256);

    for (std::uint32_t x = 0; x < chunk_size; ++x) {
        for (std::uint32_t y = 0; y < chunk_size; ++y) {
            for (std::uint32_t z = 0; z < chunk_size; ++z) {
                engine::block_t const &block = chunk.blocks[cube_at<chunk_size>(x, y, z)];

                assert(block.id < block_vertices_table.size());

                PFN_GetVertices const pfn_GetVertices = block_vertices_table[block.id];

                if (!pfn_GetVertices) continue;
                if (!is_solid(block)) continue;

                bool const is_top_visible = y == chunk_size - 1 || !(is_solid(chunk.blocks[cube_at<chunk_size>(x, y + 1, z)]) & Sides::BOTTOM);
                bool const is_bottom_visible = y == 0 || !(is_solid(chunk.blocks[cube_at<chunk_size>(x, y - 1, z)]) & Sides::TOP);
                bool const is_east_visible = x == chunk_size - 1 || !(is_solid(chunk.blocks[cube_at<chunk_size>(x + 1, y, z)]) & Sides::WEST);
                bool const is_west_visible = x == 0 || !(is_solid(chunk.blocks[cube_at<chunk_size>(x - 1, y, z)]) & Sides::EAST);
                bool const is_north_visible = z == 0 || !(is_solid(chunk.blocks[cube_at<chunk_size>(x, y, z - 1)]) & Sides::SOUTH);
                bool const is_south_visible = z == chunk_size - 1 || !(is_solid(chunk.blocks[cube_at<chunk_size>(x, y, z + 1)]) & Sides::NORTH);

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

                engine::chunk_mesh_data_t mesh = pfn_GetVertices(block, sides); // these are in block coords
                assert(mesh.indices.size() % 3 == 0);

                for (auto &vertex : mesh.vertices) // transform to world coords
                    vertex.position += static_cast<glm::vec3>(chunk.position) + glm::vec3 { x, y, z };
                for (auto &index : mesh.indices)
                    index += result.vertices.size();

                // vertices and indices are trivial so no need to move them
                result.vertices.insert(result.vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
                result.indices.insert(result.indices.end(), mesh.indices.begin(), mesh.indices.end());
            }
        }
    }

    calculate_light(result);
    remove_duplicate_vertices(result);

    return result;
}

// will be called more frequently
engine::chunk_mesh_data_t engine::generate_translucent_mesh(engine::chunk_t const &chunk)
{
    engine::chunk_mesh_data_t result;

    // avoid small allocations
    result.vertices.reserve(256);
    result.indices.reserve(256);

    // TODO!: Logic to generate translucent meshes
    //
    // Iterate over each block in chunk
    // If block is completely solid, skip
    // Check for visible sides similar to how it is done with the solid blocks, but if the adjacent block id is the same then interpret it as an invisible face
    // (perhaps check if the adjacent block is the same with the subid too, or do it at runtime, for cases like different glass color per subid.)
    // If block is not visible for any side, skip
    // Generate vertices

    calculate_light(result);
    remove_duplicate_vertices(result);

    return result;
}
