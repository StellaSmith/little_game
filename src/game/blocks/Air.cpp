#include <engine/BlockType.hpp>

static engine::BlockType s_airBlockType { "air", "Air" };
std::int32_t const AirBlockType_Registered = engine::BlockType::Register(&s_airBlockType);