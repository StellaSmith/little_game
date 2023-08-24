#ifndef ENGINE_SDL_WINDOW_HPP
#define ENGINE_SDL_WINDOW_HPP

#ifdef ENGINE_WITH_VULKAN
#include <SDL_vulkan.h>
#endif

#include <engine/sdl/Error.hpp>

#include <SDL_video.h>
#include <glm/vec2.hpp>

#include <memory>
#include <optional>

namespace engine::sdl {
    struct WindowDeleter {
        void operator()(SDL_Window *ptr) const noexcept { SDL_DestroyWindow(ptr); }
    };

    struct Window : std::unique_ptr<SDL_Window, WindowDeleter> {
        using super = std::unique_ptr<SDL_Window, WindowDeleter>;
        using super::super;

        [[nodiscard]] static Window create(char const *title, std::optional<glm::ivec2> position, glm::ivec2 size, uint32_t flags)
        {

            if (SDL_Window *raw = SDL_CreateWindow(
                    title,
                    position ? position->x : SDL_WINDOWPOS_UNDEFINED,
                    position ? position->y : SDL_WINDOWPOS_UNDEFINED,
                    size.x,
                    size.y,
                    flags))
                return Window(raw);

            throw engine::sdl::Error::current();
        }

        [[nodiscard]] static Window create(std::string const &title, std::optional<glm::ivec2> position, glm::ivec2 size, uint32_t flags)
        {
            return Window::create(title.c_str(), position, size, flags);
        }

#ifdef ENGINE_WITH_VULKAN
        struct VulkanFunctions {
            SDL_Window *raw;

            [[nodiscard]] std::vector<char const *> get_instance_extensions()
            {
                unsigned int count;
                if (SDL_Vulkan_GetInstanceExtensions(raw, &count, nullptr) == SDL_FALSE)
                    throw engine::sdl::Error::current();
                std::vector<char const *> result(count);
                if (SDL_Vulkan_GetInstanceExtensions(raw, &count, result.data()) == SDL_FALSE)
                    throw engine::sdl::Error::current();
                return result;
            }

            [[nodiscard]] VkSurfaceKHR create_surface(VkInstance instance)
            {
                VkSurfaceKHR surface;
                if (SDL_Vulkan_CreateSurface(raw, instance, &surface) == SDL_FALSE)
                    throw engine::sdl::Error::current();
                return surface;
            }
        };

        [[nodiscard]] VulkanFunctions vulkan() & noexcept
        {
            return VulkanFunctions { this->get() };
        }
#endif
#ifdef ENGINE_WITH_OPENGL
        struct OpenglContextDeleter {
            using pointer = SDL_GLContext;
            void operator()(SDL_GLContext ctx) { SDL_GL_DeleteContext(ctx); }
        };

        struct OpenglContext : std::unique_ptr<void, OpenglContextDeleter> {
            using super = std::unique_ptr<void, OpenglContextDeleter>;
            using super::super;
            explicit OpenglContext(SDL_Window *window, SDL_GLContext ctx) noexcept
                : super(ctx)
                , m_window(window)
            {
            }

            void make_current()
            {
                if (SDL_GL_MakeCurrent(m_window, this->get()) < 0)
                    throw engine::sdl::Error::current();
            }

        private:
            SDL_Window *m_window;
        };

        struct OpenglFunctions {
            SDL_Window *raw;

            OpenglContext create_context()
            {
                if (auto ctx = OpenglContext(SDL_GL_CreateContext(raw)))
                    return ctx;

                throw engine::sdl::Error::current();
            }

            void swap_buffers()
            {
                SDL_GL_SwapWindow(raw);
            }
        };

        [[nodiscard]] OpenglFunctions opengl() & noexcept
        {
            return OpenglFunctions { this->get() };
        }
#endif
    };

} // namespace engine::sdl

#endif