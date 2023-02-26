#ifndef ENGINE_BLOCKTYPE_HPP
#define ENGINE_BLOCKTYPE_HPP

#include <entt/entity/entity.hpp>

#include <vector>

namespace engine {

    struct BlockType {
        entt::id_type mesh_id = entt::null;
        std::vector<entt::id_type> texture_ids;
        std::vector<entt::id_type> masks_ids;
    };

} // namespace engine

#endif