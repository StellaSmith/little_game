#ifndef ENGINE_BLOCK_HPP
#define ENGINE_BLOCK_HPP

#include <glm/ext/scalar_uint_sized.hpp>

namespace engine {

    struct BlockType;

    struct Block {
        glm::uint32 type;
        glm::uint32 data;
    };

} // namespace engine

#endif