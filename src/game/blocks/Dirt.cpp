#include <engine/BlockType.hpp>
#include <engine/game.hpp>

struct DirtBlockType : engine::BlockType {
    int textureIndex = -1;
};

using namespace std::literals;

engine::rendering::Mesh GetVertices_Common(engine::Sides sides, int texture_index, glm::u8vec3 color = { 0xff, 0xff, 0xff });

static void DirtBlockType_Initialize(engine::BlockType *block_type, engine::Game *game)
{
    auto *self = reinterpret_cast<DirtBlockType *>(block_type);
    self->textureIndex = game->get_texture_index("dirt"sv);
}

static engine::rendering::Mesh DirtBlockType_GenerateSolidMesh(engine::BlockType const *block_type, engine::Block const *, engine::Sides visible_sides)
{
    auto *self = reinterpret_cast<DirtBlockType const *>(block_type);
    return GetVertices_Common(visible_sides, self->textureIndex);
}

static engine::Sides DirtBlockType_GetSolidSides(engine::BlockType const *, engine::Block const *)
{
    return engine::Sides::ALL;
}

static DirtBlockType s_dirtBlockType {
    "dirt_block",
    "Block of Dirt",
    &DirtBlockType_Initialize,
    &DirtBlockType_GenerateSolidMesh,
    nullptr,
    &DirtBlockType_GetSolidSides,
};

std::int32_t const DirtBlockType_Registered = engine::BlockType::Register(&s_dirtBlockType);