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

    class BlockModel {
    public:
        static BlockModel load(std::filesystem::path const &path);

        engine::rendering::Mesh const *get_solid_mesh(engine::Sides sides) const noexcept
        {
            auto const &maybe_mesh = m_meshes[sides];
            return maybe_mesh.has_value() ? &maybe_mesh.value() : nullptr;
        }

        engine::rendering::Mesh const *get_translucent_mesh(engine::Sides sides) const noexcept
        {
            auto const &maybe_mesh = m_meshes[64 + sides];
            return maybe_mesh.has_value() ? &maybe_mesh.value() : nullptr;
        }

    private:
        static BlockModel load_json(std::filesystem::path const &path);

    private:
        std::array<std::optional<engine::rendering::Mesh>, 128> m_meshes;
        std::vector<std::string> m_textures;
    };

}

#endif