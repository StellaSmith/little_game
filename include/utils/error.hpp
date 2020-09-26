#include <string_view>

struct SDL_Window;

namespace utils {
    [[noreturn]] void show_error(std::string_view msg, SDL_Window *w = nullptr) noexcept;
} // namespace utils