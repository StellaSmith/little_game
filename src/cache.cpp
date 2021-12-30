#include <utils/cache.hpp>

#include <cassert>
#include <cstdlib> // std::getenv

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <shlobj.h> // SHGetFolderPathW
// #include <windows.h>
#else
#include <pwd.h> // getpwuid
// #include <unistd.h>
#endif

std::filesystem::path const &utils::cache_directory()
{
    static std::filesystem::path cache_dir;
    if (!cache_dir.empty())
        return cache_dir;

    if (char const *const cache_env = std::getenv("CACHE_PATH"))
        return cache_dir = cache_env;

#ifdef _WIN32
    wchar_t c_homedir[MAX_PATH] {};
    SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, 0, c_homedir);
    return cache_dir = std::filesystem::path { c_homedir } / L"little_game\\cache\\";
#else

    if (char const *cache_env = std::getenv("XDG_CACHE_HOME"); cache_env != nullptr)
        return cache_dir = std::filesystem::path { cache_env } / "little_game";
    else if (char const *home_env = std::getenv("HOME"); home_env != nullptr)
        return cache_dir = std::filesystem::path { home_env } / ".cache/little_game";
    else
        return cache_dir = std::filesystem::path { getpwuid(getuid())->pw_dir } / ".cache/little_game";
#endif
}

std::FILE *utils::create_cache_file(std::string_view name)
{
    while (!name.empty() && name.back() == '/')
        name.remove_suffix(1);
    assert(!name.empty());

    std::filesystem::path const &cache_path = cache_directory();

    auto const cache_file_path = (cache_path / name).remove_filename();
    if (!std::filesystem::exists(cache_file_path)) {
        std::error_code ec;
        if (!std::filesystem::create_directories(cache_file_path, ec) || ec)
            return nullptr;
    }

    if (!std::filesystem::is_directory(cache_file_path))
        return nullptr;

    auto const cache_file = cache_path / name;

    return std::fopen(cache_file.string().c_str(), "wb");
}

std::FILE *utils::get_cache_file(absl::string_view name, absl::Span<absl::string_view> ref_files)
{
    while (!name.empty() && name.back() == '/')
        name.remove_suffix(1);
    assert(!name.empty());

    std::filesystem::path const &cache_path = cache_directory();

    auto cache_file = cache_path / name;

    if (!std::filesystem::is_regular_file(cache_file))
        return nullptr;

    if (ref_files.empty())
        return std::fopen(cache_file.string().c_str(), "rb");

    std::error_code ec;
    auto cache_time = std::filesystem::last_write_time(cache_file, ec).time_since_epoch().count();
    if (ec)
        return nullptr;

    auto max_time = std::filesystem::last_write_time(ref_files.front(), ec).time_since_epoch().count();
    if (ec)
        return nullptr;

    for (auto it = ref_files.begin() + 1; it != ref_files.end(); ++it) {
        max_time = std::max(max_time, std::filesystem::last_write_time(*it, ec).time_since_epoch().count());
        if (ec)
            return nullptr;
    }

    if (max_time < cache_time)
        return std::fopen(cache_file.string().c_str(), "rb");

    return nullptr;
}