
#include <engine/BlockType.hpp>

void engine::BlockType::Initialize(engine::Game &) { }

engine::rendering::Mesh engine::BlockType::GetSolidMesh(engine::Block const &, engine::Sides) const { return {}; }

engine::rendering::Mesh engine::BlockType::GetTranslucentMesh(engine::Block const &, engine::Sides) const { return {}; }

engine::Sides engine::BlockType::GetSolidSides(engine::Block const &) const { return engine::Sides::NONE; }

glm::u8vec4 engine::BlockType::GetProducedLight(engine::Block const &) const { return { 0, 0, 0, 0 }; }

engine::BlockType::~BlockType() { }