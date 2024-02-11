#ifndef ENGINE_SDL_ERROR_HPP
#define ENGINE_SDL_ERROR_HPP

#include <SDL_error.h>
#include <fmt/core.h>

#include <cstring>
#include <exception>
#include <memory>

namespace engine::sdl {

    class Error final : public std::runtime_error {
    private:
        Error(char const *message)
            : std::runtime_error(message)
        {
        }

    public:
        [[nodiscard]]
        static Error current()
        {
            return Error(SDL_GetError());
        }

        template <typename... T>
        static Error set_current(fmt::format_string<T...> format, T &&...args)
        {
            {
                std::string const error_string = fmt::format(std::move(format), std::forward<T>(args)...);
                SDL_SetError("%.*s", static_cast<int>(error_string.size()), error_string.data());
            }

            return Error::current();
        }
    };

} // namespace engine::sdl

#endif