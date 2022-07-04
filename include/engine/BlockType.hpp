#ifndef ENGINE_BLOCKTYPE_HPP
#define ENGINE_BLOCKTYPE_HPP

#include <entt/entity/entity.hpp>

#include <boost/container/small_vector.hpp>

namespace engine {

    struct BlockType {
        entt::entity mesh_id = entt::null;
        boost::container::small_vector<entt::entity, 4> texture_ids;
        boost::container::small_vector<entt::entity, 4> masks_ids;
    };

} // namespace engine

#endif