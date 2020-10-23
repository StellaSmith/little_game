#include "engine/chunk_mesh_generation.hpp"
#include "engine/chunk_t.hpp"
#include "engine/game.hpp"
#include "engine/rendering/block.hpp"
#include "math/bits.hpp"
#include "math/constexpr.hpp"
#include "utils/timeit.hpp"

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
    case 1: // colorful
    case 2: // stone
    case 3: // dirt
    case 4: // grass
        return Sides::ALL;
    default:
        return Sides::NONE; // we don't know
    }
}

constexpr auto chunk_size = engine::chunk_t::chunk_size;

static Sides get_visible_sides(engine::chunk_t const &chunk, glm::u32vec3 block_pos)
{
    auto const [x, y, z] = block_pos;
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
    return sides;
}

using vertex = engine::rendering::block_vertex_t;
using vertex_vector = std::vector<vertex>;
using index_vector = std::vector<std::uint32_t>;
using PFN_GetVertices = engine::chunk_mesh_data_t (*)(engine::Game const &, engine::block_t const &, Sides);

// most blocks are very simple, like stone, dirt, rock, etc
// might template on texture coordinates?
using namespace std::literals;
static std::vector<std::string_view> id_to_name = {
    {},
    "white"sv,
    "stone"sv,
    "dirt"sv
};
static engine::chunk_mesh_data_t GetVertices_Common(engine::Game const &game, engine::block_t const &block, Sides sides)
{
    engine::chunk_mesh_data_t result;
    if (block.id >= id_to_name.size() || id_to_name[block.id].empty()) return result;
    auto const texture_name = id_to_name[block.id];
    int const texture_index = game.get_texture_index(texture_name);

    if (sides & Sides::TOP) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { -0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index } },
                vertex { { -0.5f, +0.5f, +0.5f }, { 0.0f, 1.0f, texture_index } },
                vertex { { +0.5f, +0.5f, +0.5f }, { 1.0f, 1.0f, texture_index } },
                vertex { { +0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, texture_index } },
            });
        result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
    }

    if (sides & Sides::NORTH) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index } }, // 0
                vertex { { +0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, texture_index } }, // 1
                vertex { { -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, texture_index } }, // 2
                vertex { { -0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, texture_index } }, // 3
            });
        result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
    }

    if (sides & Sides::WEST) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { -0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index } }, // 0
                vertex { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, texture_index } }, // 1
                vertex { { -0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index } }, // 2
                vertex { { -0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, texture_index } }, // 3
            });
        result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
    }

    if (sides & Sides::SOUTH) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { +0.5f, +0.5f, +0.5f }, { 0.0f, 0.0f, texture_index } }, // 0
                vertex { { +0.5f, -0.5f, +0.5f }, { 0.0f, 1.0f, texture_index } }, // 1
                vertex { { -0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index } }, // 2
                vertex { { -0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, texture_index } }, // 3
            });
        result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
    }

    if (sides & Sides::EAST) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index } }, // 0
                vertex { { +0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, texture_index } }, // 1
                vertex { { +0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index } }, // 2
                vertex { { +0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, texture_index } }, // 3
            });
        result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
    }

    if (sides & Sides::BOTTOM) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, texture_index } }, // 0
                vertex { { -0.5f, -0.5f, +0.5f }, { 0.0f, 1.0f, texture_index } }, // 1
                vertex { { +0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index } }, // 2
                vertex { { +0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, texture_index } }, // 3
            });
        result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
    }
    return result;
}

static engine::chunk_mesh_data_t GetVertices_Colorful(engine::Game const &game, engine::block_t const &block, Sides sides)
{
    glm::u8vec4 color = math::unpack_u32(static_cast<std::uint32_t>(block.data.u64));
    engine::chunk_mesh_data_t result = GetVertices_Common(game, block, sides);
    for (auto &vertex : result.vertices)
        vertex.color = glm::u8vec3 { color.x, color.y, color.z };
    return result;
}

// grass uses tree textures:
//  bottom being the same as the base for dirt,
//  top begin the green one,
//  and for sides a green to dirt
static engine::chunk_mesh_data_t GetVertices_Grass(engine::Game const &game, [[maybe_unused]] engine::block_t const &block, [[maybe_unused]] Sides sides)
{
    // TODO!: return the vertices of a grass block
    engine::chunk_mesh_data_t result;
    bool const top_visible = sides & Sides::TOP;
    bool const sides_visible = sides & (Sides::NORTH | Sides::SOUTH | Sides::EAST | Sides::WEST);
    bool const bottom_visible = sides & Sides::BOTTOM;
    glm::u8vec3 const color { 0x00, 0xFF, 0x55 };

    int const texture_index_top = top_visible ? game.get_texture_index("grass_block_top") : -1; // grass top
    int const texture_index_side1 = sides_visible ? game.get_texture_index("dirt" /*"grass_block_side1"*/) : -1; // dirt side
    int const texture_index_side2 = sides_visible ? game.get_texture_index("grass_block_side2") : -1; // grass side
    int const texture_index_bottom = bottom_visible ? game.get_texture_index("dirt") : -1; // dirt bottom

    if (top_visible) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { -0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index_top }, color }, // 0
                vertex { { -0.5f, +0.5f, +0.5f }, { 0.0f, 1.0f, texture_index_top }, color }, // 1
                vertex { { +0.5f, +0.5f, +0.5f }, { 1.0f, 1.0f, texture_index_top }, color }, // 2
                vertex { { +0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, texture_index_top }, color }, // 3
            });

        result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
    }

    if (sides & Sides::NORTH) {
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index_side2 }, color }, // 0
                    vertex { { +0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, texture_index_side2 }, color }, // 1
                    vertex { { -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, texture_index_side2 }, color }, // 2
                    vertex { { -0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, texture_index_side2 }, color }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
        }
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index_side1 } }, // 0
                    vertex { { +0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, texture_index_side1 } }, // 1
                    vertex { { -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, texture_index_side1 } }, // 2
                    vertex { { -0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, texture_index_side1 } }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
        }
    }

    if (sides & Sides::WEST) {
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { -0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index_side2 }, color }, // 0
                    vertex { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, texture_index_side2 }, color }, // 1
                    vertex { { -0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index_side2 }, color }, // 2
                    vertex { { -0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, texture_index_side2 }, color }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
        }
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { -0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index_side1 } }, // 0
                    vertex { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, texture_index_side1 } }, // 1
                    vertex { { -0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index_side1 } }, // 2
                    vertex { { -0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, texture_index_side1 } }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
        }
    }

    if (sides & Sides::SOUTH) {
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { +0.5f, +0.5f, +0.5f }, { 0.0f, 0.0f, texture_index_side2 }, color }, // 0
                    vertex { { +0.5f, -0.5f, +0.5f }, { 0.0f, 1.0f, texture_index_side2 }, color }, // 1
                    vertex { { -0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index_side2 }, color }, // 2
                    vertex { { -0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, texture_index_side2 }, color }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
        }
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { +0.5f, +0.5f, +0.5f }, { 0.0f, 0.0f, texture_index_side1 } }, // 0
                    vertex { { +0.5f, -0.5f, +0.5f }, { 0.0f, 1.0f, texture_index_side1 } }, // 1
                    vertex { { -0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index_side1 } }, // 2
                    vertex { { -0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, texture_index_side1 } }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
        }
    }
    if (sides & Sides::EAST) {
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index_side2 }, color }, // 0
                    vertex { { +0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, texture_index_side2 }, color }, // 1
                    vertex { { +0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index_side2 }, color }, // 2
                    vertex { { +0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, texture_index_side2 }, color }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
        }
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index_side1 } }, // 0
                    vertex { { +0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, texture_index_side1 } }, // 1
                    vertex { { +0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index_side1 } }, // 2
                    vertex { { +0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, texture_index_side1 } }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
        }
    }

    if (bottom_visible) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, texture_index_bottom } }, // 0
                vertex { { -0.5f, -0.5f, +0.5f }, { 0.0f, 1.0f, texture_index_bottom } }, // 1
                vertex { { +0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index_bottom } }, // 2
                vertex { { +0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, texture_index_bottom } }, // 3
            });
        result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
    }
    return result;
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

static void remove_duplicate_vertices(engine::chunk_mesh_data_t &chunk_data)
{
    assert(chunk_data.vertices.size() <= UINT32_MAX);
    if (chunk_data.vertices.empty()) [[unlikely]] return;
    for (std::uint32_t i = 0; i < chunk_data.vertices.size() - 1; ++i) {
        auto it = std::find_if(chunk_data.vertices.data() + i + 1, chunk_data.vertices.data() + chunk_data.vertices.size(), [to_find = chunk_data.vertices.data() + i](auto const &vertex) {
            return std::memcmp(to_find, &vertex, sizeof(vertex)) == 0;
        });
        if (it == chunk_data.vertices.data() + chunk_data.vertices.size()) continue;
        auto found_index = static_cast<std::uint32_t>(it - chunk_data.vertices.data());
        std::replace(chunk_data.indices.data(), chunk_data.indices.data() + chunk_data.indices.size(), found_index, i);
        chunk_data.vertices.erase(chunk_data.vertices.begin() + found_index);
    }
}

static void calculate_light(engine::chunk_t const &chunk, engine::chunk_mesh_data_t &mesh_data)
{
    for (std::uint32_t x = 0; x < chunk_size; ++x) {
        for (std::uint32_t y = 0; y < chunk_size; ++y) {
            for (std::uint32_t z = 0; z < chunk_size; ++z) {
                if (!get_visible_sides(chunk, { x, y, z })) continue;
                engine::block_t const &block = chunk.blocks.data()[cube_at<chunk_size>(x, y, z)];
                glm::u8vec4 const produced_light = get_produced_light(block);
                if (!produced_light.w) continue;
                for (auto &vertex : mesh_data.vertices) {
                    float const distance = std::max(glm::length(vertex.position - glm::vec3 { x, y, z } + static_cast<glm::vec3>(chunk.position)), 1.0f);
                    if (distance > produced_light.w) continue;

                    glm::u8vec3 const light { produced_light.x / distance, produced_light.y / distance, produced_light.z / distance };
                    vertex.light.x = std::max(light.x, vertex.light.x);
                    vertex.light.y = std::max(light.y, vertex.light.y);
                    vertex.light.z = std::max(light.z, vertex.light.z);
                }
            }
        }
    }
}

static void remove_unreferenced_vertices(engine::chunk_mesh_data_t &mesh_data)
{
    // there shouldn't be that many anyways
    assert(mesh_data.vertices.size() <= std::numeric_limits<std::make_signed_t<std::size_t>>::max());

    std::vector<bool> mask(mesh_data.vertices.size(), false);
    for (auto const &index : mesh_data.indices)
        mask[index] = true;

    for (std::make_signed_t<std::size_t> i = mesh_data.vertices.size() - 1; i >= 0; --i)
        if (!mask[i])
            mesh_data.vertices.erase(mesh_data.vertices.begin() + i);
}

engine::chunk_mesh_data_t engine::generate_solid_mesh(engine::Game const &game, engine::chunk_t const &chunk)
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

                Sides sides = get_visible_sides(chunk, { x, y, z });
                if (!sides) continue; // no visible sides

                engine::chunk_mesh_data_t mesh = pfn_GetVertices(game, block, sides); // these are in block coords
                assert(mesh.indices.size() % 3 == 0);

                for (auto &vertex : mesh.vertices) // transform to chunk coords
                    vertex.position += glm::vec3 { x, y, z };

                for (auto &index : mesh.indices)
                    index += result.vertices.size();

                // vertices and indices are trivial so no need to move them
                result.vertices.insert(result.vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
                result.indices.insert(result.indices.end(), mesh.indices.begin(), mesh.indices.end());
            }
        }
    }

    using namespace std::literals;
    // {
    //     utils::TimeIt timer { "vertex duplication removal"sv };
    //     remove_duplicate_vertices(result);
    // }
    {
        utils::TimeIt timer { "unreferenced vertex removal"sv };
        remove_unreferenced_vertices(result);
    }
    {
        utils::TimeIt timer { "lights"sv };
        calculate_light(chunk, result);
    }

    for (auto &vertex : result.vertices) // transform to world coords
        vertex.position += static_cast<glm::vec3>(chunk.position) * static_cast<float>(chunk_size);

    return result;
}

// will be called more frequently
engine::chunk_mesh_data_t engine::generate_translucent_mesh(engine::Game const &game, engine::chunk_t const &chunk)
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

    calculate_light(chunk, result);
    remove_duplicate_vertices(result);
    remove_unreferenced_vertices(result);

    return result;
}
