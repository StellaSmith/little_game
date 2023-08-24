#ifndef ENGINE_ASSETS_IMAGE_HPP
#define ENGINE_ASSETS_IMAGE_HPP

#include <utils/memory.hpp>

#include <SDL_surface.h>

#include <filesystem>
#include <memory>
#include <span>

namespace engine::assets {

    class Image {
    public:
        static Image load(std::filesystem::path const &path);
        static Image load(std::span<std::byte const> bytes);

        [[nodiscard]] std::uint32_t width() const noexcept { return m_surface->w; }
        [[nodiscard]] std::uint32_t height() const noexcept { return m_surface->h; }

        [[nodiscard]] void *data() noexcept { return m_surface->pixels; }
        [[nodiscard]] void const *data() const noexcept { return m_surface->pixels; }

    private:
        struct SurfaceDeleter {
            void operator()(SDL_Surface *ptr) const noexcept
            {
                SDL_FreeSurface(ptr);
            }
        };
        std::unique_ptr<SDL_Surface, SurfaceDeleter> m_surface;
    };

}

#endif