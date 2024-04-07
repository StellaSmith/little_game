#ifndef ENGINE_ASSETS_BASEASSET_HPP
#define ENGINE_ASSETS_BASEASSET_HPP

#include <filesystem>

namespace engine::assets {

    class IAsset {
    private:
        std::string_view m_name;

    public:
        IAsset(std::string_view name)
            : m_name(name)
        {
        }

        [[nodiscard]]
        std::string_view name() const noexcept
        {
            return m_name;
        }

        virtual void load(std::filesystem::path const &) = 0;

        virtual ~IAsset();

        IAsset(IAsset const &) = delete;
        IAsset &operator=(IAsset const &) = delete;
        IAsset(IAsset &&) = delete;
        IAsset &operator=(IAsset &&) = delete;
    };

} // namespace engine::assets

#endif
