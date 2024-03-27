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
#include <vector>

namespace engine::sdl {

    struct Window {
    private:
        struct Deleter {
            using pointer = SDL_Window *;
            void operator()(pointer ptr) const noexcept
            {
                SDL_DestroyWindow(ptr);
            }
        };
        std::unique_ptr<SDL_Window, Deleter> m_raw;

    public:
        explicit Window(SDL_Window *raw) noexcept
            : m_raw(raw)
        {
        }

        [[nodiscard]]
        static Window create(char const *title, std::optional<glm::ivec2> position, glm::ivec2 size, std::uint32_t flags)
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

        [[nodiscard]]
        static Window create(std::string const &title, std::optional<glm::ivec2> position, glm::ivec2 size, std::uint32_t flags)
        {
            return Window::create(title.c_str(), position, size, flags);
        }

        [[nodiscard]]
        std::uint32_t id() const noexcept
        {
            return SDL_GetWindowID(m_raw.get());
        }

        [[nodiscard]]
        SDL_Window *raw() const noexcept
        {
            return m_raw.get();
        }

#ifdef ENGINE_WITH_VULKAN
        struct VulkanFunctions {
            Window &window;

            [[nodiscard]]
            std::vector<char const *> get_instance_extensions()
            {
                unsigned int count;
                if (SDL_Vulkan_GetInstanceExtensions(window.m_raw.get(), &count, nullptr) == SDL_FALSE)
                    throw engine::sdl::Error::current();
                std::vector<char const *> result(count);
                if (SDL_Vulkan_GetInstanceExtensions(window.m_raw.get(), &count, result.data()) == SDL_FALSE)
                    throw engine::sdl::Error::current();
                return result;
            }

            [[nodiscard]]
            VkSurfaceKHR create_surface(VkInstance instance)
            {
                VkSurfaceKHR surface;
                if (SDL_Vulkan_CreateSurface(window.m_raw.get(), instance, &surface) == SDL_FALSE)
                    throw engine::sdl::Error::current();
                return surface;
            }
        };

        [[nodiscard]]
        VulkanFunctions vulkan() & noexcept
        {
            return VulkanFunctions { *this };
        }
#endif

#ifdef ENGINE_WITH_OPENGL

        struct OpenGLContext {
        private:
            struct Deleter {
                using pointer = SDL_GLContext;
                void operator()(pointer ctx) const noexcept
                {
                    SDL_GL_DeleteContext(ctx);
                }
            };

            std::unique_ptr<std::remove_pointer_t<Deleter::pointer>, Deleter> m_raw;
            SDL_Window *m_window;

        public:
            explicit OpenGLContext(SDL_Window *window, SDL_GLContext ctx) noexcept
                : m_raw(ctx)
                , m_window(window)
            {
            }

            void make_current()
            {
                if (SDL_GL_MakeCurrent(m_window, m_raw.get()) < 0)
                    throw engine::sdl::Error::current();
            }

            [[nodiscard]]
            SDL_GLContext raw() const noexcept
            {
                return m_raw.get();
            }
        };

        struct OpenGLFunctions {
            Window &window;

            OpenGLContext create_context()
            {
                if (auto raw = SDL_GL_CreateContext(window.m_raw.get()))
                    return OpenGLContext(window.m_raw.get(), raw);
                throw engine::sdl::Error::current();
            }

            void swap_buffers()
            {
                SDL_GL_SwapWindow(window.m_raw.get());
            }
        };

        [[nodiscard]]
        OpenGLFunctions opengl() & noexcept
        {
            return OpenGLFunctions { *this };
        }
#endif
    };

} // namespace engine::sdl

#endif