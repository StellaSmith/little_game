#ifndef ENGINE_BLOCKTYPE_HPP
#define ENGINE_BLOCKTYPE_HPP

#include <engine/Block.hpp>
#include <engine/rendering/Mesh.hpp>

#include <algorithm>
#include <glm/vec4.hpp>
#include <string_view>
#include <vector>

#include <macros.h>

namespace engine {
    class Game;

    struct BlockType {
        std::string_view name;
        std::string_view displayName;

        // default (null) does nothing
        void (*ALL_NONNULL initialize)(BlockType *, Game *) = nullptr;

        // default (null) returns empty
        engine::rendering::Mesh (*ALL_NONNULL generateSolidMesh)(BlockType const *, Block const *, engine::Sides) = nullptr;

        // default (null) returns empty
        engine::rendering::Mesh (*ALL_NONNULL generateTranslucentMesh)(BlockType const *, Block const *, engine::Sides) = nullptr;

        // default (null) returns NONE
        engine::Sides (*ALL_NONNULL getSolidSides)(BlockType const *, Block const *) = nullptr;

        // default (null) returns {0, 0, 0, 0}
        glm::u8vec4 (*ALL_NONNULL getProducedLight)(BlockType const *, Block const *) = nullptr;

        ALL_NONNULL
        static std::int32_t Register(BlockType *block_type)
        {
            if (GetRegisteredByName(block_type->name))
                return -1;
            s_registeredBlockTypes.push_back(block_type);
            return static_cast<std::int32_t>(s_registeredBlockTypes.size() - 1);
        }

        [[nodiscard]] static std::vector<BlockType *> const &GetRegistered()
        {
            return s_registeredBlockTypes;
        }

        [[nodiscard]] static BlockType *GetRegisteredByName(std::string_view name) noexcept
        {
            auto start = s_registeredBlockTypes.data();
            auto stop = s_registeredBlockTypes.data() + s_registeredBlockTypes.size();
            auto *const ptr = std::find_if(start, stop, [name](BlockType *block_type) {
                return block_type->name == name;
            });

            if (ptr == stop)
                return nullptr;

            return *ptr;
        }

        [[nodiscard]] static BlockType *GetRegisteredById(std::uint32_t id) noexcept
        {
            if (id >= s_registeredBlockTypes.size())
                return nullptr;
            return s_registeredBlockTypes.data()[id];
        }

        [[nodiscard]] static std::int32_t GetRegisteredIdByName(std::string_view name) noexcept
        {
            auto start = s_registeredBlockTypes.data();
            auto stop = s_registeredBlockTypes.data() + s_registeredBlockTypes.size();
            auto *const ptr = std::find_if(start, stop, [name](BlockType *block_type) {
                return block_type->name == name;
            });
            if (ptr == stop)
                return -1;
            return static_cast<std::int32_t>(ptr - s_registeredBlockTypes.data());
        }

    private:
        static std::vector<BlockType *> s_registeredBlockTypes;
    };

} // namespace engine

#endif