#include <engine/BlockType.hpp>
#include <engine/game.hpp>

struct StoneBlockType : engine::BlockType {
    int textureIndex = -1;
};

using engine::Sides;
using namespace std::literals;

engine::rendering::Mesh GetVertices_Common(Sides sides, int texture_index, glm::u8vec3 color = { 0xff, 0xff, 0xff })
{
    using engine::rendering::Mesh;
    using vertex = Mesh::vertex_t;
    Mesh result;

    if (sides & Sides::TOP) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { -0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index }, color },
                vertex { { -0.5f, +0.5f, +0.5f }, { 0.0f, 1.0f, texture_index }, color },
                vertex { { +0.5f, +0.5f, +0.5f }, { 1.0f, 1.0f, texture_index }, color },
                vertex { { +0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, texture_index }, color },
            });
        result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
    }

    if (sides & Sides::NORTH) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index }, color }, // 0
                vertex { { +0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, texture_index }, color }, // 1
                vertex { { -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, texture_index }, color }, // 2
                vertex { { -0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, texture_index }, color }, // 3
            });
        result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
    }

    if (sides & Sides::WEST) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { -0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index }, color }, // 0
                vertex { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, texture_index }, color }, // 1
                vertex { { -0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index }, color }, // 2
                vertex { { -0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, texture_index }, color }, // 3
            });
        result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
    }

    if (sides & Sides::SOUTH) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { +0.5f, +0.5f, +0.5f }, { 0.0f, 0.0f, texture_index }, color }, // 0
                vertex { { +0.5f, -0.5f, +0.5f }, { 0.0f, 1.0f, texture_index }, color }, // 1
                vertex { { -0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index }, color }, // 2
                vertex { { -0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, texture_index }, color }, // 3
            });
        result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
    }

    if (sides & Sides::EAST) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, texture_index }, color }, // 0
                vertex { { +0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, texture_index }, color }, // 1
                vertex { { +0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index }, color }, // 2
                vertex { { +0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, texture_index }, color }, // 3
            });
        result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
    }

    if (sides & Sides::BOTTOM) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, texture_index }, color }, // 0
                vertex { { -0.5f, -0.5f, +0.5f }, { 0.0f, 1.0f, texture_index }, color }, // 1
                vertex { { +0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, texture_index }, color }, // 2
                vertex { { +0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, texture_index }, color }, // 3
            });
        result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
    }
    return result;
}

static void StoneBlockType_Initialize(engine::BlockType *block_type, engine::Game *game)
{
    auto *self = reinterpret_cast<StoneBlockType *>(block_type);
    self->textureIndex = game->get_texture_index("stone"sv);
}

static engine::rendering::Mesh StoneBlockType_GenerateSolidMesh(engine::BlockType const *block_type, engine::Block const *, Sides visible_sides)
{
    auto *self = reinterpret_cast<StoneBlockType const *>(block_type);
    return GetVertices_Common(visible_sides, self->textureIndex);
}

static engine::Sides StoneBlockType_GetSolidSides(engine::BlockType const *, engine::Block const *)
{
    return engine::Sides::ALL;
}

static StoneBlockType s_stoneBlockType {
    "stone_block",
    "Block of Stone",
    &StoneBlockType_Initialize,
    &StoneBlockType_GenerateSolidMesh,
    nullptr,
    &StoneBlockType_GetSolidSides,
};

std::int32_t const StoneBlockType_Registered = engine::BlockType::Register(&s_stoneBlockType);