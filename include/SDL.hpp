#ifndef SDL2_HPP
#define SDL2_HPP

#include <SDL.h>

#include <functional>
#include <memory>
#include <stdexcept>

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

class SDL {
public:
    class Error : public std::runtime_error {
    private:
        using std::runtime_error::runtime_error;

    public:
        static Error current()
        {
            return Error(SDL_GetError());
        }
    };

    explicit SDL(std::uint32_t flags)
    {
        if (SDL_Init(flags) < 0)
            throw Error::current();
    }

    explicit SDL()
        : SDL(SDL_INIT_EVERYTHING)
    {
    }

    class Audio {
    public:
        static auto drivers()
        {
            int n = SDL_GetNumAudioDrivers();
            if (n < 0)
                throw SDL::Error::current();
            return ::sdl_impl::CountedIterator(n, [](std::size_t i) {
                return SDL_GetAudioDriver(i);
            });
        }
    };

    class Video {
    public:
        static auto drivers()
        {
            int n = SDL_GetNumVideoDrivers();
            if (n < 0)
                throw SDL::Error::current();
            return ::sdl_impl::CountedIterator(n, [](std::size_t i) {
                return SDL_GetVideoDriver(i);
            });
        }

        class Window;

        struct OpenGLContextDeleter {
            void operator()(void *gl) const noexcept
            {
                SDL_GL_DeleteContext(gl);
            }
        };

        class OpenGLContext : public std::unique_ptr<void, OpenGLContextDeleter> {
        public:
            using base_type = std::unique_ptr<void, OpenGLContextDeleter>;
            using base_type::base_type;

            void make_current()
            {
                if (SDL_GL_MakeCurrent(win, get()) < 0)
                    throw Error::current();
            }

        private:
            friend class Window;
            SDL_Window *win;
        };

        struct WindowDeleter {
            void operator()(SDL_Window *win) const noexcept
            {
                SDL_DestroyWindow(win);
            }
        };

        class Window : public std::unique_ptr<SDL_Window, WindowDeleter> {
        public:
            using base_type = std::unique_ptr<SDL_Window, WindowDeleter>;
            using base_type::base_type;

            class Builder {
            public:
                Window create()
                {
                    SDL_GL_ResetAttributes();
                    std::for_each(gl_attributes.cbegin(), gl_attributes.cend(), [](auto const &p) {
                        if (SDL_GL_SetAttribute(p.first, p.second) < 0)
                            throw SDL::Error::current();
                    });

                    auto result = Window(SDL_CreateWindow(title.c_str(), x, y, w, h, flags));
                    if (!result)
                        throw SDL::Error::current();
                    return result;
                }

                template <typename S>
                Builder set_title(S &&title) &&
                {
                    this->title = std::string(std::forward<S>(title));
                    return std::move(*this);
                }

                template <typename S>
                Builder set_title(S &&title) const &
                {
                    auto copy = *this;
                    copy.title = std::string(std::forward<S>(title));
                    copy.title = title;
                    return copy;
                }

                Builder set_gl_attribute(SDL_GLattr attr, int value) &&
                {
                    this->gl_attributes.emplace_back(attr, value);
                    return std::move(*this);
                }

                Builder set_gl_attribute(SDL_GLattr attr, int value) const &
                {
                    auto copy = *this;
                    copy.gl_attributes.emplace_back(attr, value);
                    return copy;
                }

                template <typename It, typename Sentinel>
                Builder set_gl_attributes(It start, Sentinel end) &&
                {
                    if constexpr (std::is_same_v<typename std::iterator_traits<It>::iterator_category, std::random_access_iterator_tag>) {
                        this->gl_attributes.reserve(gl_attributes.size() + std::distance(start, end));
                    }

                    for (; start != end; ++start) {
                        decltype(auto) pair = *start;
                        this->gl_attributes.emplace_back(std::get<0>(pair), std::get<1>(pair));
                    }

                    return std::move(*this);
                }

                Builder set_dimensions(int w, int h) &&
                {
                    this->w = w;
                    this->h = h;
                    return std::move(*this);
                }

                Builder set_dimensions(int w, int h) const &
                {
                    auto copy = *this;
                    copy.w = w;
                    copy.h = h;
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
                Builder opengl() &&
                {
                    return std::move(*this).set_flag<SDL_WINDOW_OPENGL>();
                }

                Builder opengl() const &
                {
                    return set_flag<SDL_WINDOW_OPENGL>();
                }

                Builder resizable() &&
                {
                    return std::move(*this).set_flag<SDL_WINDOW_RESIZABLE>();
                }

                Builder resizable() const &
                {
                    return set_flag<SDL_WINDOW_RESIZABLE>();
                }

                Builder shown() &&
                {
                    return std::move(*this).unset_flag<SDL_WINDOW_HIDDEN>();
                }

                Builder shown() const &
                {
                    return unset_flag<SDL_WINDOW_HIDDEN>();
                }

                std::vector<std::pair<SDL_GLattr, int>> gl_attributes;
                std::string title;
                int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED, w = 800, h = 600;
                std::uint32_t flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN;
            };

            OpenGLContext opengl_context()
            {
                auto context = OpenGLContext(SDL_GL_CreateContext(get()));
                if (!context)
                    throw Error::current();
                context.win = get();
                return context;
            }

            static Builder builder(Video &)
            {
                return Builder {};
            }
        };

        Window::Builder window_builder()
        {
            return Window::builder(*this);
        }
    };

    Audio &init_audio(char const *driver = nullptr)
    {
        driver = driver && *driver != 0 ? driver : nullptr;
        if (SDL_AudioInit(driver) < 0)
            throw Error::current();
        to_quit |= QuitAudio;
        return audio();
    }

    [[nodiscard]] Audio &audio()
    {
        static Audio audio {};
        if (!SDL_WasInit(SDL_INIT_AUDIO))
            if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
                throw Error::current();
        return audio;
    }

    Video &init_video(char const *driver = nullptr)
    {
        driver = driver && *driver != 0 ? driver : nullptr;
        if (SDL_VideoInit(driver) < 0)
            throw Error::current();
        to_quit |= QuitVideo;
        return video();
    }

    [[nodiscard]] Video &video()
    {
        static Video video {};
        if (!SDL_WasInit(SDL_INIT_VIDEO))
            if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
                throw Error::current();
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
    enum {
        QuitAudio,
        QuitVideo
    };

    int to_quit = 0;
};

#endif