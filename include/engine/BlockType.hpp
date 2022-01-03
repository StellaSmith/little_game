#ifndef ENGINE_BLOCKTYPE_HPP
#define ENGINE_BLOCKTYPE_HPP

#include <glm/ext/vector_uint4_sized.hpp>

namespace engine {

    struct BlockType {
        glm::uint32 model_id;
        glm::u8vec4 produced_light;
    };

} // namespace engine

#endif