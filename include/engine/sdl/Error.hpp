#ifndef ENGINE_SDL_ERROR_HPP
#define ENGINE_SDL_ERROR_HPP

#include <SDL_error.h>
#include <fmt/core.h>
#include <fmt/format.h>

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

        static Error current(std::string_view message)
        {
            SDL_SetError("%.*s", static_cast<int>(message.size()), message.data());
            return Error::current();
        }

        template <typename... T>
        static Error current(fmt::format_string<T...> format, T &&...args)
        {
            fmt::memory_buffer buffer;
            fmt::format_to(fmt::appender(buffer), std::move(format), std::forward<T>(args)...);
            return Error::current(std::string_view(buffer.data(), buffer.size()));
        }
    };

} // namespace engine::sdl

#endif