#ifndef UTILS_STRINGS_HPP
#define UTILS_STRINGS_HPP

#include <string_view>

namespace utils {

    constexpr std::string_view lstrip(std::string_view str, char chars) noexcept
    {
        while (!str.empty() && chars == str.front())
            str.remove_prefix(1);
        return str;
    }

    constexpr std::string_view lstrip(std::string_view str, std::string_view chars = "\t\v \r\n") noexcept
    {
        while (!str.empty() && chars.find(str.front()) == std::string_view::npos)
            str.remove_prefix(1);
        return str;
    }

    constexpr std::string_view rstrip(std::string_view str, char chars) noexcept
    {
        while (!str.empty() && chars == str.back())
            str.remove_suffix(1);
        return str;
    }

    constexpr std::string_view rstrip(std::string_view str, std::string_view chars = "\t\v \r\n") noexcept
    {
        while (!str.empty() && chars.find(str.back()) == std::string_view::npos)
            str.remove_suffix(1);
        return str;
    }

    constexpr std::string_view strip(std::string_view str, std::string_view chars = "\t\v \r\n") noexcept
    {
        return lstrip(rstrip(str, chars), chars);
    }

    constexpr std::string_view strip(std::string_view str, char chars) noexcept
    {
        return lstrip(rstrip(str, chars), chars);
    }

    constexpr bool ends_with(std::wstring_view str, std::wstring_view suffix) noexcept
    {
        if (str.size() >= suffix.size())
            return str.substr(str.size() - suffix.size()) == suffix;
        else
            return false;
    }

    constexpr bool ends_with(std::string_view str, std::string_view suffix) noexcept
    {
        if (str.size() >= suffix.size())
            return str.substr(str.size() - suffix.size()) == suffix;
        else
            return false;
    }

}

#endif