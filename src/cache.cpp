#include <utils/cache.hpp>

#include <engine/Stream.hpp>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cstdlib> // std::getenv
#include <optional>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <shlobj.h> // SHGetFolderPathW
#else
#include <pwd.h> // getpwuid
#include <unistd.h> // getuid
#endif

using namespace std::literals;

static std::filesystem::path const s_cache_directory = []() -> std::filesystem::path {
    if (char const *const cache_env = std::getenv("CACHE_PATH"))
        return cache_env;
    else if (char const *cache_env = std::getenv("XDG_CACHE_HOME"); cache_env != nullptr)
        return std::filesystem::path { cache_env } / "little_game"sv;
#ifdef _WIN32
    wchar_t c_homedir[MAX_PATH] {};
    SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, nullptr, 0, c_homedir);
    return cache_dir = std::filesystem::path(c_homedir, std::filesystem::path::native_format) / L"little_game\\cache\\"sv;
#else
    else if (char const *home_env = std::getenv("HOME"); home_env != nullptr)
        return std::filesystem::path { home_env } / ".cache/little_game"sv;
    else
        return std::filesystem::path(getpwuid(getuid())->pw_dir, std::filesystem::path::native_format) / ".cache/little_game"sv;
#endif
}();

std::filesystem::path const &utils::cache_directory()
{
    return s_cache_directory;
}

static utils::FileHandle open_cache_file(std::string_view name, char const *mode)
{
    while (!name.empty() && name.back() == '/')
        name.remove_suffix(1);
    assert(!name.empty());

    auto const cache_file = s_cache_directory / name;
    auto const cache_file_directory = cache_file.parent_path();

    if (!cache_file_directory.lexically_relative(s_cache_directory).native().starts_with(s_cache_directory.native())) {
        spdlog::warn("Tried to open a cache file outside the cache directory: {}", name);
        return nullptr;
    }

    if (!std::filesystem::exists(cache_file_directory)) {
        std::error_code ec;
        if (!std::filesystem::create_directories(cache_file_directory, ec) || ec)
            return nullptr;
    }

    if (!std::filesystem::is_directory(cache_file_directory))
        return nullptr;

    return engine::open_file(cache_file, mode);
}

utils::FileHandle utils::create_cache_file(std::string_view name)
{
    return open_cache_file(name, "wb");
}

utils::FileHandle utils::get_cache_file(std::string_view name, std::span<std::filesystem::path const> ref_files)
{
    if (ref_files.empty())
        return open_cache_file(name, "rb");

    std::error_code ec;
    auto cache_time = std::filesystem::last_write_time(s_cache_directory / name, ec);
    if (ec) return nullptr;

    auto const maybe_max_time = [&] {
        std::vector<std::filesystem::file_time_type> times;
        times.reserve(ref_files.size());

        for (auto const &ref_file : ref_files) {
            std::error_code ec;
            auto write_time = std::filesystem::last_write_time(ref_file, ec);
            if (!ec)
                times.push_back(write_time);
        }

        auto it = std::max_element(times.begin(), times.end());
        if (it == times.end())
            return std::optional<std::filesystem::file_time_type> { std::nullopt };
        else
            return std::optional<std::filesystem::file_time_type>(std::in_place, std::move(*it));
    }();

    if (maybe_max_time.has_value() && maybe_max_time.value() < cache_time)
        return open_cache_file(name, "rb");

    return nullptr;
}