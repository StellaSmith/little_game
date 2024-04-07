#ifndef ENGINE_ASSETS_BLOCKMODEL_HPP
#define ENGINE_ASSETS_BLOCKMODEL_HPP

#include <engine/OptionalArray.hpp>
#include <engine/Sides.hpp>
#include <engine/assets/IAsset.hpp>
#include <engine/rendering/Mesh.hpp>

#include <boost/container/small_vector.hpp>

#include <filesystem>
#include <string>

namespace engine::assets {

    class BlockMesh : public IAsset {
    public:
        explicit BlockMesh(std::string_view name);
        void load(std::filesystem::path const &path) override;

        void load_json(std::filesystem::path const &path);

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
            return m_meshes.has(i);
        }

        engine::rendering::Mesh const *get_mesh(std::size_t i) const noexcept
        {
            return m_meshes.at(i);
        }

        engine::rendering::Mesh *get_mesh(std::size_t i) noexcept
        {
            return m_meshes.at(i);
        }

        engine::rendering::Mesh &get_or_emplace(std::size_t i) noexcept
        {
            return m_meshes.emplace(i);
        }

    private:
        engine::OptionalArray<engine::rendering::Mesh, 128> m_meshes;
        boost::container::small_vector<std::uint32_t, 4> m_textures;
    };

}

#endif