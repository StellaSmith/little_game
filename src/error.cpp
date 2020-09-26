#include "utils/error.hpp"
#include <SDL.h>

[[noreturn]] void utils::show_error(std::string_view msg, SDL_Window *w) noexcept
{
    if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR!", msg.data(), w) < 0)
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", msg.data());
    std::exit(EXIT_FAILURE);
}