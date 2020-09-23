#include "utils/cache.hpp"

#include <cassert>
#include <filesystem>

std::FILE *utils::create_cache_file(std::string_view name)
{
    while (!name.empty() && name.back() == '/')
        name.remove_suffix(1);

    assert(!name.empty());

    static std::filesystem::path const cache_path = [] {
        char const *const cache_env = std::getenv("CACHE_PATH");
        std::filesystem::path p = cache_env ? cache_env : ".cache";
        return p;
    }();

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

std::FILE *utils::get_cache_file(std::string_view name, std::vector<std::string_view> ref_files)
{
    while (!name.empty() && name.back() == '/')
        name.remove_suffix(1);

    assert(!name.empty());

    static std::filesystem::path const cache_path = [] {
        char const *const cache_env = std::getenv("CACHE_PATH");
        std::filesystem::path p = cache_env ? cache_env : ".cache";
        return p;
    }();

    auto cache_file = cache_path / name;

    if (!std::filesystem::is_regular_file(cache_file))
        return nullptr;

    if (ref_files.empty())
        return std::fopen(cache_file.string().c_str(), "rb");

    std::error_code ec;
    auto cache_time = std::filesystem::last_write_time(cache_file, ec).time_since_epoch().count();
    if (ec) return nullptr;
    
    auto max_time = std::filesystem::last_write_time(ref_files.front(), ec).time_since_epoch().count();
    if (ec) return nullptr;

    for (auto it = ref_files.begin() + 1; it != ref_files.end(); ++it) {
        max_time = std::max(max_time, std::filesystem::last_write_time(*it, ec).time_since_epoch().count());
        if (ec) return nullptr;
    }

    if (max_time < cache_time)
        return std::fopen(cache_file.string().c_str(), "rb");

    return nullptr;
}