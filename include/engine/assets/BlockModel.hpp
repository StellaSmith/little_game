#ifndef ENGINE_ASSETS_BLOCKMODEL_HPP
#define ENGINE_ASSETS_BLOCKMODEL_HPP

#include <engine/Sides.hpp>
#include <engine/rendering/Mesh.hpp>

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace engine::assets {

    class BlockModel {
    public:
        static BlockModel load(std::string_view path);

        engine::rendering::Mesh const &get_solid_mesh(engine::Sides sides) const noexcept
        {
            return m_meshes[sides];
        }

        engine::rendering::Mesh const &get_translucent_mesh(engine::Sides sides) const noexcept
        {
            return m_meshes[64 + sides];
        }

    private:
        static BlockModel load_json(std::string_view path);

    private:
        std::array<engine::rendering::Mesh, 128> m_meshes;
        std::vector<std::string> m_textures;
    };

}

#endif