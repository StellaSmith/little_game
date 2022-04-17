#ifndef ENGINE_ASSETS_BLOCKMODEL_HPP
#define ENGINE_ASSETS_BLOCKMODEL_HPP

#include <engine/Sides.hpp>
#include <engine/rendering/Mesh.hpp>

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace engine::assets {

    class BlockMesh {
    public:
        static BlockMesh load(std::filesystem::path const &path);

        engine::rendering::Mesh const *get_solid_mesh(engine::Sides sides) const noexcept
        {
            auto const i = static_cast<std::size_t>(sides);
            return get_mesh(i);
        }

        engine::rendering::Mesh const *get_translucent_mesh(engine::Sides sides) const noexcept
        {
            auto const i = 64 + static_cast<std::size_t>(sides);
            return get_mesh(i);
        }

    private:
        bool has_mesh(std::size_t i) const noexcept
        {
            return m_bits[i / CHAR_BIT] & (1 << (i % CHAR_BIT));
        }

        engine::rendering::Mesh const *get_mesh(std::size_t i) const noexcept
        {
            if (has_mesh(i))
                return std::launder(&m_meshes[i].storage);
            else
                return nullptr;
        }

        engine::rendering::Mesh *get_mesh(std::size_t i) noexcept
        {
            if (has_mesh(i))
                return std::launder(&m_meshes[i].storage);
            else
                return nullptr;
        }

        engine::rendering::Mesh &get_or_emplace(std::size_t i) noexcept
        {
            if (has_mesh(i))
                return *std::launder(&m_meshes[i].storage);
            else
                return *new (&m_meshes[i].storage) engine::rendering::Mesh;
        }

        static BlockMesh load_json(std::filesystem::path const &path);

    private:
        struct MaybeMesh {
            engine::rendering::Mesh storage;
        };

        char m_bits[128 / CHAR_BIT] {};
        MaybeMesh m_meshes[128];
        std::vector<std::uint32_t> m_textures;
    };

}

#endif