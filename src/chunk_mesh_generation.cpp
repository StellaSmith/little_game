#include "engine/chunk_mesh_generation.hpp"
#include "engine/Chunk.hpp"
#include "engine/game.hpp"
#include "math/bits.hpp"
#include "math/constexpr.hpp"
#include "utils/timeit.hpp"
#include <engine/BlockType.hpp>
#include <engine/rendering/Mesh.hpp>

#include <glad/gl.h>

#include <algorithm>
#include <cstring>
#include <type_traits>
#include <vector>

#include <engine/Block.hpp>

#if defined(__GNUC__) || defined(__clang__)
#define OPTIMIZE __attribute__((optimize("Ofast")))
#else
#define OPTIMIZE
#endif

template <std::size_t D, typename First, typename... Rest, typename std::enable_if_t<std::is_integral_v<std::common_type_t<First, Rest...>>, std::nullptr_t> = nullptr>
constexpr static std::size_t cube_at(First first, Rest... rest)
{
    if constexpr (sizeof...(Rest) == 0)
        return first;
    else
        return first * math::c_ipow_v<D, sizeof...(Rest)> + cube_at<D>(rest...);
}

static engine::Sides get_visible_sides(engine::Chunk const &chunk, std::vector<engine::BlockType *> const &block_type_table, glm::u32vec3 block_pos)
{
    using engine::Sides;
    constexpr auto chunk_size = engine::Chunk::chunk_size;

    auto const is_solid = [block_type_table](engine::Block const &block) -> bool {
        auto const block_type = block_type_table[block.id];
        auto pfn_getSolidSides = block_type->getSolidSides;
        if (!pfn_getSolidSides) return engine::Sides::NONE;
        return pfn_getSolidSides(block_type_table[block.id], &block);
    };
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

static glm::u8vec4 get_produced_light(engine::Block const &block) // r, g, b, intensity
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

static void remove_duplicate_vertices(engine::rendering::Mesh &chunk_data)
{
    assert(chunk_data.vertices.size() <= UINT32_MAX);
    if (chunk_data.vertices.empty()) [[unlikely]]
        return;
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

static void OPTIMIZE calculate_light(engine::Chunk const &chunk, engine::rendering::Mesh &mesh_data)
{
    struct LightData {
        glm::vec3 position;
        glm::u8vec4 light;
    };

    std::vector<LightData> lights;
    lights.reserve(64);

    constexpr auto chunk_size = engine::Chunk::chunk_size;
    for (std::uint_fast32_t i = 0; i < chunk_size * chunk_size * chunk_size; ++i) {
        std::uint_fast8_t const x = i >> 8 & 0xF;
        std::uint_fast8_t const y = i >> 4 & 0xF;
        std::uint_fast8_t const z = i >> 0 & 0xF;
        if (!get_visible_sides(chunk, engine::BlockType::GetRegistered(), { x, y, z }))
            continue;
        engine::Block const &block = chunk.blocks.data()[i];
        glm::u8vec4 const produced_light = get_produced_light(block);
        if (!produced_light.w)
            continue;

        lights.push_back({ { x, y, z }, produced_light });
    }

    for (auto &vertex : mesh_data.vertices) {
        for (auto const &light_data : lights) {
            float const distance = std::max(glm::length(vertex.position - light_data.position), 1.0f);
            if (distance > light_data.light.w) continue;

            glm::u8vec3 const light { light_data.light.x / distance, light_data.light.y / distance, light_data.light.z / distance };
            vertex.light.x = std::max(light.x, vertex.light.x);
            vertex.light.y = std::max(light.y, vertex.light.y);
            vertex.light.z = std::max(light.z, vertex.light.z);
        }
    }
}

static void remove_unreferenced_vertices(engine::rendering::Mesh &mesh_data)
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

engine::rendering::Mesh engine::generate_solid_mesh(engine::Chunk const &chunk)
{
    engine::rendering::Mesh result;

    // avoid small allocations
    result.vertices.reserve(256);
    result.indices.reserve(256);

    std::vector<engine::BlockType *> const block_type_table = BlockType::GetRegistered();

    constexpr auto chunk_size = engine::Chunk::chunk_size;
    for (std::uint_fast32_t i = 0; i < chunk_size * chunk_size * chunk_size; ++i) {
        std::uint_fast8_t const x = i >> 8 & 0xF;
        std::uint_fast8_t const y = i >> 4 & 0xF;
        std::uint_fast8_t const z = i >> 0 & 0xF;

        engine::Block const &block = chunk.blocks.data()[i];

        BlockType const *block_type = block_type_table.data()[block.id];
        if (block_type->generateSolidMesh == nullptr)
            continue;
        if (block_type->getSolidSides == nullptr)
            continue;
        if (block_type->getSolidSides(block_type, &block) == Sides::NONE)
            continue;

        Sides sides = get_visible_sides(chunk, block_type_table, { x, y, z });
        if (!sides) // no visible sides
            continue;

        engine::rendering::Mesh mesh = block_type->generateSolidMesh(block_type, &block, sides); // these are in block coords
        assert(mesh.indices.size() % 3 == 0);

        for (auto &vertex : mesh.vertices) // transform to chunk coords
            vertex.position += glm::vec3 { x, y, z };

        for (auto &index : mesh.indices)
            index += result.vertices.size();

        // vertices and indices are trivial so no need to move them
        result.vertices.insert(result.vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
        result.indices.insert(result.indices.end(), mesh.indices.begin(), mesh.indices.end());
    }

    using namespace std::literals;
    // {
    //     utils::TimeIt timer { "solid vertex deduplication"sv };
    //     remove_duplicate_vertices(result);
    // }
    {
        utils::TimeIt timer { "solid unreferenced vertex removal"sv };
        remove_unreferenced_vertices(result);
    }
    {
        utils::TimeIt timer { "solid lights"sv };
        calculate_light(chunk, result);
    }

    for (auto &vertex : result.vertices) // transform to world coords
        vertex.position += static_cast<glm::vec3>(chunk.position) * static_cast<float>(chunk_size);

    return result;
}

// will be called more frequently
engine::rendering::Mesh engine::generate_translucent_mesh(engine::Chunk const &chunk)
{
    engine::rendering::Mesh result;

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

    using namespace std::literals;
    {
        utils::TimeIt timer { "translucent vertex duplication removal"sv };
        remove_duplicate_vertices(result);
    }
    {
        utils::TimeIt timer { "translucent unreferenced vertex removal"sv };
        remove_unreferenced_vertices(result);
    }
    {
        utils::TimeIt timer { "translucent lights"sv };
        calculate_light(chunk, result);
    }

    return result;
}
