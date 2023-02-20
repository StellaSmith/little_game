#ifndef SDL2_HPP
#define SDL2_HPP

#include <SDL.h>

#include <algorithm>
#include <exception>
#include <tl/expected.hpp>

#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

namespace sdl_impl {

    template <typename F>
    struct CountedIterator {
    private:
        std::size_t max;
        std::size_t cur;
        F fun;

    public:
        CountedIterator(std::size_t max, F fun)
            : max { max }
            , cur { 0 }
            , fun(std::move(fun))
        {
        }

        auto operator*() const
        {
            return std::invoke(fun, std::size_t { cur });
        }

        CountedIterator &operator++() noexcept
        {
            ++cur;
            return *this;
        }

        bool operator==(std::default_sentinel_t) const noexcept
        {
            return cur == max;
        }

        CountedIterator begin() &&
        {
            return std::move(*this);
        }

        CountedIterator begin() const &
        {
            return *this;
        }

        std::default_sentinel_t end() const &noexcept { return {}; }

        std::size_t size() const noexcept
        {
            return max;
        }
    };

}

struct SDL {

    struct Error : std::exception {
    private:
        char const *m_str;
        explicit Error(char const *str) noexcept
            : m_str(str)
        {
        }

    public:
        [[nodiscard]] static Error current() noexcept
        {
            return Error(SDL_GetError());
        }

        char const *what() const noexcept override
        {
            return m_str;
        }
    };

    [[nodiscard]] static tl::expected<SDL, Error> create(std::uint32_t flags) noexcept
    {
        if (SDL_Init(flags) < 0)
            return tl::make_unexpected(Error::current());
        return {};
    }

    [[nodiscard]] static tl::expected<SDL, Error> create() noexcept
    {
        return SDL::create(SDL_INIT_EVERYTHING);
    }

    struct Audio {
        [[nodiscard]] static tl::expected<sdl_impl::CountedIterator<decltype(&SDL_GetAudioDriver)>, Error> drivers() noexcept
        {
            if (int n = SDL_GetNumAudioDrivers(); n < 0)
                return tl::make_unexpected(Error::current());
            else
                return sdl_impl::CountedIterator(n, &SDL_GetAudioDriver);
        }
    };

    struct Video {
        [[nodiscard]] static tl::expected<sdl_impl::CountedIterator<decltype(&SDL_GetVideoDriver)>, Error> drivers() noexcept
        {
            if (int n = SDL_GetNumAudioDrivers(); n < 0)
                return tl::make_unexpected(Error::current());
            else
                return sdl_impl::CountedIterator(n, &SDL_GetVideoDriver);
        }

        struct Window;

    private:
        struct OpenGLContextDeleter {
            void operator()(void *gl) const noexcept
            {
                SDL_GL_DeleteContext(gl);
            }
        };

    public:
        struct OpenGLContext : public std::unique_ptr<void, OpenGLContextDeleter> {

            using base_type = std::unique_ptr<void, OpenGLContextDeleter>;
            using base_type::base_type;

            [[nodiscard]] tl::expected<void, Error> make_current(SDL_Window *window) noexcept
            {
                if (SDL_GL_MakeCurrent(window, get()) < 0)
                    return tl::make_unexpected(Error::current());
                return {};
            }

            [[nodiscard]] tl::expected<void, Error> make_current(Window &window) noexcept
            {
                return make_current(window.get());
            }
        };

    private:
        struct WindowDeleter {
            using pointer = SDL_Window *;
            void operator()(SDL_Window *window) const noexcept
            {
                SDL_DestroyWindow(window);
            }
        };

    public:
        struct Window : std::unique_ptr<SDL_Window, WindowDeleter> {
            using base_type = std::unique_ptr<SDL_Window, WindowDeleter>;
            using base_type::base_type;

            struct Builder {
                [[nodiscard]] tl::expected<Window, Error> build() noexcept
                {
                    SDL_GL_ResetAttributes();
                    for (auto const &[name, value] : gl_attributes) {
                        if (SDL_GL_SetAttribute(name, value) < 0)
                            return tl::make_unexpected(Error::current());
                    }

                    auto result = Window(SDL_CreateWindow(title.c_str(), x, y, width, height, flags));
                    if (!result)
                        return tl::make_unexpected(Error::current());
                    return std::move(result);
                }

                template <typename S>
                [[nodiscard]] Builder set_title(S &&title) &&
                {
                    this->title = static_cast<std::string>(std::forward<S>(title));
                    return std::move(*this);
                }

                template <typename S>
                [[nodiscard]] Builder set_title(S &&title) const &
                {
                    auto copy = *this;
                    copy.title = static_cast<std::string>(std::forward<S>(title));
                    return copy;
                }

                [[nodiscard]] Builder set_gl_attribute(SDL_GLattr attr, int value) &&
                {
                    this->gl_attributes.emplace_back(attr, value);
                    return std::move(*this);
                }

                [[nodiscard]] Builder set_gl_attribute(SDL_GLattr attr, int value) const &
                {
                    auto copy = *this;
                    copy.gl_attributes.emplace_back(attr, value);
                    return copy;
                }

                template <typename It, typename Sentinel>
                [[nodiscard]] Builder set_gl_attributes(It start, Sentinel end) &&
                {
                    if constexpr (std::is_same_v<typename std::iterator_traits<It>::iterator_category, std::random_access_iterator_tag>) {
                        this->gl_attributes.reserve(gl_attributes.size() + std::distance(start, end));
                    }

                    for (; start != end; ++start) {
                        auto &&[name, value] = *start;
                        this->gl_attributes.emplace_back(static_cast<decltype(name) &&>(name), static_cast<decltype(value) &&>(value));
                    }

                    return std::move(*this);
                }

                [[nodiscard]] Builder set_dimensions(int width, int height) &&noexcept
                {
                    this->width = width;
                    this->height = height;
                    return std::move(*this);
                }

                [[nodiscard]] Builder set_dimensions(int width, int height) const &noexcept
                {
                    auto copy = *this;
                    copy.width = width;
                    copy.height = height;
                    return copy;
                }

            private:
                template <std::uint32_t flag>
                Builder set_flag() &&
                {
                    this->flags |= flag;
                    return std::move(*this);
                }

                template <std::uint32_t flag>
                Builder set_flag() const &
                {
                    auto copy = *this;
                    copy.flags |= flag;
                    return copy;
                }

                template <std::uint32_t flag>
                Builder unset_flag() &&
                {
                    this->flags &= ~flag;
                    return std::move(*this);
                }

                template <std::uint32_t flag>
                Builder unset_flag() const &
                {
                    auto copy = *this;
                    copy.flags &= ~flag;
                    return copy;
                }

            public:
                [[nodiscard]] Builder with_opengl() &&noexcept
                {
                    return std::move(*this).set_flag<SDL_WINDOW_OPENGL>();
                }

                [[nodiscard]] Builder with_opengl() const &noexcept
                {
                    return set_flag<SDL_WINDOW_OPENGL>();
                }

                [[nodiscard]] Builder with_vulkan() &&noexcept
                {
                    return std::move(*this).set_flag<SDL_WINDOW_VULKAN>();
                }

                [[nodiscard]] Builder with_vulkan() const &noexcept
                {
                    return set_flag<SDL_WINDOW_OPENGL>();
                }

                [[nodiscard]] Builder resizable(bool enabled = true) &&noexcept
                {
                    if (enabled)
                        return std::move(*this).set_flag<SDL_WINDOW_RESIZABLE>();
                    else
                        return std::move(*this).unset_flag<SDL_WINDOW_RESIZABLE>();
                }

                [[nodiscard]] Builder resizable(bool enabled = true) const &noexcept
                {
                    if (enabled)
                        return set_flag<SDL_WINDOW_RESIZABLE>();
                    else
                        return unset_flag<SDL_WINDOW_RESIZABLE>();
                }

                [[nodiscard]] Builder hidden(bool enabled = true) &&noexcept
                {
                    if (enabled)
                        return std::move(*this).set_flag<SDL_WINDOW_HIDDEN>();
                    else
                        return std::move(*this).unset_flag<SDL_WINDOW_HIDDEN>();
                }

                [[nodiscard]] Builder hidden(bool enabled = true) const &noexcept
                {
                    if (enabled)
                        return set_flag<SDL_WINDOW_HIDDEN>();
                    else
                        return unset_flag<SDL_WINDOW_HIDDEN>();
                }

                [[nodiscard]] Builder shown(bool enabled = true) &&noexcept
                {
                    return std::move(*this).hidden(!enabled);
                }

                [[nodiscard]] Builder shown(bool enabled = true) const &noexcept
                {
                    return hidden(!enabled);
                }

                std::vector<std::pair<SDL_GLattr, int>> gl_attributes;
                std::string title;
                int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;
                int width = 800, height = 600;
                std::uint32_t flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN;
            };

            [[nodiscard]] tl::expected<OpenGLContext, Error> opengl_context() noexcept
            {
                auto context = OpenGLContext(SDL_GL_CreateContext(get()));
                if (!context)
                    return tl::make_unexpected(Error::current());
                return std::move(context);
            }

            [[nodiscard]] static Builder builder(Video &video) noexcept
            {
                return Builder {};
            }
        };

        [[nodiscard]] Window::Builder window_builder() noexcept
        {
            return Window::builder(*this);
        }
    };

    [[nodiscard]] tl::expected<std::reference_wrapper<Audio>, Error> init_audio(char const *driver = nullptr) noexcept
    {
        driver = driver && *driver != 0 ? driver : nullptr;
        if (SDL_AudioInit(driver) < 0)
            return tl::make_unexpected(Error::current());
        to_quit |= QuitAudio;
        return audio();
    }

    [[nodiscard]] tl::expected<std::reference_wrapper<Audio>, Error> audio() noexcept
    {
        static Audio audio {};
        if (!SDL_WasInit(SDL_INIT_AUDIO))
            if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
                return tl::make_unexpected(Error::current());
        return audio;
    }

    [[nodiscard]] tl::expected<std::reference_wrapper<Video>, Error> init_video(char const *driver = nullptr) noexcept
    {
        driver = driver && *driver != 0 ? driver : nullptr;
        if (SDL_VideoInit(driver) < 0)
            return tl::make_unexpected(Error::current());
        to_quit |= QuitVideo;
        return video();
    }

    [[nodiscard]] tl::expected<std::reference_wrapper<Video>, Error> video() noexcept
    {
        static Video video {};
        if (!SDL_WasInit(SDL_INIT_VIDEO))
            if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
                return tl::make_unexpected(Error::current());
        return video;
    }

    ~SDL()
    {
        if (to_quit & QuitVideo)
            SDL_VideoQuit();
        if (to_quit & QuitAudio)
            SDL_AudioQuit();
        SDL_Quit();
    }

private:
    enum : unsigned {
        QuitAudio = 1 << 0,
        QuitVideo = 1 << 1
    };

    unsigned to_quit = 0;
};

#endif