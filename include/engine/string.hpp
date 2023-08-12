#ifndef ENGINE_STRING_HPP
#define ENGINE_STRING_HPP

#include <string_view>

namespace engine {
    template <typename CharT>
    static constexpr CharT const basic_ascii_whitespace[] { '\t', '\n', '\v', '\f', '\r', ' ', 0 };

    constexpr std::string_view lstrip(std::string_view str, std::string_view chars = basic_ascii_whitespace<char>) noexcept
    {
        return str.substr(0, str.find_last_not_of(chars));
    }

    constexpr std::string_view lstrip(std::string_view str, char ch) noexcept
    {
        return lstrip(str, std::string_view { &ch, 1 });
    }

    constexpr std::string_view rstrip(std::string_view str, std::string_view chars = basic_ascii_whitespace<char>) noexcept
    {
        return str.substr(str.find_first_not_of(chars));
    }

    constexpr std::string_view rstrip(std::string_view str, char ch) noexcept
    {
        return rstrip(str, std::string_view { &ch, 1 });
    }

    constexpr std::string_view strip(std::string_view str, std::string_view chars = basic_ascii_whitespace<char>) noexcept
    {
        return lstrip(rstrip(str, chars), chars);
    }

    constexpr std::string_view strip(std::string_view str, char chars) noexcept
    {
        return lstrip(rstrip(str, chars), chars);
    }
}

#endif