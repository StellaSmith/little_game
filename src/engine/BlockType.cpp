
#include <engine/BlockType.hpp>

std::vector<engine::BlockType *> engine::BlockType::s_registeredBlockTypes = []() {
    std::vector<engine::BlockType *> result;
    result.reserve(512);
    return result;
}();