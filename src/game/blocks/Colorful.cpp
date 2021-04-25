#include <engine/BlockType.hpp>
#include <engine/game.hpp>
#include <math/bits.hpp>

struct ColorfulBlockType : engine::BlockType {
    int textureIndex = -1;
};

using engine::Sides;

static void ColorfulBlockType_Initialize(engine::BlockType *block_type, engine::Game *game)
{
    auto *self = reinterpret_cast<ColorfulBlockType *>(block_type);
    self->textureIndex = game->get_texture_index("white");
}

engine::rendering::Mesh GetVertices_Common(Sides sides, int texture_index, glm::u8vec3 color);

static engine::rendering::Mesh ColorfulBlockType_GenerateSolidMesh(engine::BlockType const *block_type, engine::Block const *block, Sides visible_sides)
{
    auto *self = reinterpret_cast<ColorfulBlockType const *>(block_type);
    glm::u8vec4 color = math::unpack_u32(block->subid);
    return GetVertices_Common(visible_sides, self->textureIndex, { color.x, color.y, color.z });
}

static engine::Sides ColorfulBlockType_GetSolidSides(engine::BlockType const *, engine::Block const *)
{
    return engine::Sides::ALL;
}

static glm::u8vec4 ColorfulBlockType_GetProducedLight(engine::BlockType const *, engine::Block const *block)
{
    constexpr float default_intensity = 16.0f;
    auto color = math::unpack_u32(block->subid);
    float const intensity = glm::length(glm::vec3 { color.x, color.y, color.z } / 255.0f);
    color.w = static_cast<std::uint8_t>(intensity * default_intensity);
    return color;
    return { color.x, color.y, color.z, 16 };
}

static ColorfulBlockType s_colorfulBlockType {
    "colorful_block",
    "Block of Color",
    &ColorfulBlockType_Initialize,
    &ColorfulBlockType_GenerateSolidMesh,
    nullptr,
    &ColorfulBlockType_GetSolidSides,
    &ColorfulBlockType_GetProducedLight
};

std::int32_t const ColorfulBlockType_Registered = engine::BlockType::Register(&s_colorfulBlockType);