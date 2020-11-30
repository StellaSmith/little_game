#include <engine/BlockType.hpp>
#include <engine/game.hpp>

struct GrassBlockType : engine::BlockType {
    int dirtTextureIndex = -1;
    int grassTopTextureIndex = -1;
    int grassSideTextureIndex = -1;
};

using namespace std::literals;

engine::rendering::Mesh GetVertices_Grass(engine::Sides sides, int dirtTextureIndex, int grassTopTextureIndex, int grassSideTextureIndex, glm::u8vec3 color = { 0x00, 0xFF, 0x55 })
{
    using vertex = engine::rendering::Mesh::vertex_t;
    engine::rendering::Mesh result;

    if (sides & engine::Sides::TOP) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { -0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, grassTopTextureIndex }, color }, // 0
                vertex { { -0.5f, +0.5f, +0.5f }, { 0.0f, 1.0f, grassTopTextureIndex }, color }, // 1
                vertex { { +0.5f, +0.5f, +0.5f }, { 1.0f, 1.0f, grassTopTextureIndex }, color }, // 2
                vertex { { +0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, grassTopTextureIndex }, color }, // 3
            });

        result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
    }

    if (sides & engine::Sides::NORTH) {
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, grassSideTextureIndex }, color }, // 0
                    vertex { { +0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, grassSideTextureIndex }, color }, // 1
                    vertex { { -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, grassSideTextureIndex }, color }, // 2
                    vertex { { -0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, grassSideTextureIndex }, color }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
        }
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, dirtTextureIndex } }, // 0
                    vertex { { +0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, dirtTextureIndex } }, // 1
                    vertex { { -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, dirtTextureIndex } }, // 2
                    vertex { { -0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, dirtTextureIndex } }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
        }
    }

    if (sides & engine::Sides::WEST) {
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { -0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, grassSideTextureIndex }, color }, // 0
                    vertex { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, grassSideTextureIndex }, color }, // 1
                    vertex { { -0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, grassSideTextureIndex }, color }, // 2
                    vertex { { -0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, grassSideTextureIndex }, color }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
        }
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { -0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, dirtTextureIndex } }, // 0
                    vertex { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, dirtTextureIndex } }, // 1
                    vertex { { -0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, dirtTextureIndex } }, // 2
                    vertex { { -0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, dirtTextureIndex } }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 0, offset + 1, offset + 3, offset + 3, offset + 1, offset + 2 });
        }
    }

    if (sides & engine::Sides::SOUTH) {
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { +0.5f, +0.5f, +0.5f }, { 0.0f, 0.0f, grassSideTextureIndex }, color }, // 0
                    vertex { { +0.5f, -0.5f, +0.5f }, { 0.0f, 1.0f, grassSideTextureIndex }, color }, // 1
                    vertex { { -0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, grassSideTextureIndex }, color }, // 2
                    vertex { { -0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, grassSideTextureIndex }, color }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
        }
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { +0.5f, +0.5f, +0.5f }, { 0.0f, 0.0f, dirtTextureIndex } }, // 0
                    vertex { { +0.5f, -0.5f, +0.5f }, { 0.0f, 1.0f, dirtTextureIndex } }, // 1
                    vertex { { -0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, dirtTextureIndex } }, // 2
                    vertex { { -0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, dirtTextureIndex } }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
        }
    }

    if (sides & engine::Sides::EAST) {
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, grassSideTextureIndex }, color }, // 0
                    vertex { { +0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, grassSideTextureIndex }, color }, // 1
                    vertex { { +0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, grassSideTextureIndex }, color }, // 2
                    vertex { { +0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, grassSideTextureIndex }, color }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
        }
        {
            auto offset = static_cast<std::uint32_t>(result.vertices.size());
            result.vertices.insert(result.vertices.end(),
                {
                    vertex { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, dirtTextureIndex } }, // 0
                    vertex { { +0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, dirtTextureIndex } }, // 1
                    vertex { { +0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, dirtTextureIndex } }, // 2
                    vertex { { +0.5f, +0.5f, +0.5f }, { 1.0f, 0.0f, dirtTextureIndex } }, // 3
                });
            result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
        }
    }

    if (sides & engine::Sides::BOTTOM) {
        auto offset = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.insert(result.vertices.end(),
            {
                vertex { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, dirtTextureIndex } }, // 0
                vertex { { -0.5f, -0.5f, +0.5f }, { 0.0f, 1.0f, dirtTextureIndex } }, // 1
                vertex { { +0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, dirtTextureIndex } }, // 2
                vertex { { +0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, dirtTextureIndex } }, // 3
            });
        result.indices.insert(result.indices.end(), { offset + 3, offset + 1, offset + 0, offset + 2, offset + 1, offset + 3 });
    }

    return result;
}

static void GrassBlockType_Initialize(engine::BlockType *block_type, engine::Game *game)
{
    auto *self = reinterpret_cast<GrassBlockType *>(block_type);
    self->dirtTextureIndex = game->get_texture_index("dirt");
    self->grassTopTextureIndex = game->get_texture_index("grass_block_top");
    self->grassSideTextureIndex = game->get_texture_index("grass_block_side2");
}

static engine::rendering::Mesh GrassBlockType_GenerateSolidMesh(engine::BlockType const *block_type, engine::Block const *, engine::Sides visible_sides)
{
    auto *self = reinterpret_cast<GrassBlockType const *>(block_type);
    return GetVertices_Grass(visible_sides, self->dirtTextureIndex, self->grassTopTextureIndex, self->grassSideTextureIndex);
}

static engine::Sides GrassBlockType_GetSolidSides(engine::BlockType const *, engine::Block const *)
{
    return engine::Sides::ALL;
}

static GrassBlockType s_grassBlockType {
    "grass_block",
    "Block of Grass",
    &GrassBlockType_Initialize,
    &GrassBlockType_GenerateSolidMesh,
    nullptr,
    &GrassBlockType_GetSolidSides,
};

std::int32_t const GrassBlockType_Registered = engine::BlockType::Register(&s_grassBlockType);