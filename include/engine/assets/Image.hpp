#ifndef ENGINE_ASSETS_IMAGE_HPP
#define ENGINE_ASSETS_IMAGE_HPP

#include <engine/assets/IAsset.hpp>
#include <engine/sdl/Error.hpp>
#include <utils/memory.hpp>

#include <SDL_surface.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <span>

namespace engine::assets {

    class Image : public engine::assets::IAsset {
    public:
        explicit Image(std::string_view);

        void load(std::filesystem::path const &path) override;
        void load(std::span<std::byte const> bytes, std::optional<std::string_view> format = std::nullopt);

        void save(std::filesystem::path const &path, std::string_view format = "png") const;
        void save(std::span<std::byte> bytes, std::string_view format = "png") const;

        [[nodiscard]]
        std::uint32_t width() const noexcept
        {
            return m_surface->w;
        }

        [[nodiscard]]
        std::uint32_t height() const noexcept
        {
            return m_surface->h;
        }

        [[nodiscard]]
        std::uint32_t pitch() const noexcept
        {
            return m_surface->pitch;
        }

        struct Lock {
            explicit Lock(SDL_Surface *surface)
                : m_surface(surface)
            {
                if (SDL_LockSurface(surface) != 0)
                    throw engine::sdl::Error::current();
            }

            ~Lock()
            {
                SDL_UnlockSurface(m_surface);
            }

            [[nodiscard]]
            std::byte *pixels() const noexcept
            {
                return static_cast<std::byte *>(m_surface->pixels);
            }

        private:
            SDL_Surface *m_surface;
        };

        [[nodiscard]]
        Lock lock() const noexcept
        {
            return Lock { m_surface.get() };
        }

    private:
        struct Deleter {
            using pointer = SDL_Surface *;
            void operator()(pointer ptr) const noexcept
            {
                SDL_FreeSurface(ptr);
            }
        };
        std::unique_ptr<SDL_Surface, Deleter> m_surface;
    };

}

#endif
