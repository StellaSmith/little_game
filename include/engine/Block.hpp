#ifndef ENGINE_BLOCK_HPP
#define ENGINE_BLOCK_HPP

#include <entt/core/fwd.hpp>

namespace engine {

    struct Block {
        entt::id_type type_id = entt::null;
        entt::id_type data_id = entt::null;
    };

} // namespace engine

#endif