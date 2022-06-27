#ifndef ENGINE_BLOCK_HPP
#define ENGINE_BLOCK_HPP

#include <entt/core/fwd.hpp>
#include <entt/entity/entity.hpp>

namespace engine {

    struct Block {
        entt::entity type_id = entt::null;
        entt::id_type data = entt::null;
    };

} // namespace engine

#endif