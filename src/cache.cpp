#include <engine/Stream.hpp>
#include <engine/cache.hpp>

#include <boost/outcome/success_failure.hpp>
#include <fmt/std.h>
#include <spdlog/spdlog.h>

#include <algorithm> // std::max
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
    else {
        wchar_t c_homedir[MAX_PATH] {};
        SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, nullptr, 0, c_homedir);
        return std::filesystem::path(c_homedir, std::filesystem::path::native_format) / L"little_game\\cache\\"sv;
    }
#else
    else if (char const *home_env = std::getenv("HOME"); home_env != nullptr) {
        return std::filesystem::path { home_env } / ".cache/little_game"sv;
    } else {
        return std::filesystem::path(getpwuid(getuid())->pw_dir, std::filesystem::path::native_format) / ".cache/little_game"sv;
    }
#endif
}();

std::filesystem::path const &engine::cache_directory() noexcept
{
    return s_cache_directory;
}

static engine::Result<utils::FileHandle> open_cache_file(std::string_view name, char const *mode)
{
    while (!name.empty() && name.back() == '/')
        name.remove_suffix(1);

    if (name.empty())
        return boost::outcome_v2::failure(std::errc::invalid_argument);

    auto const cache_file_path = s_cache_directory / name;
    auto const cache_file_directory = cache_file_path.parent_path();

    if (!cache_file_directory.lexically_relative(s_cache_directory).native().starts_with(s_cache_directory.native())) {
        spdlog::error("tried to open a cache file outside the cache directory: {} resolved to {}", name, cache_file_path);
        return boost::outcome_v2::failure(std::errc::permission_denied);
    }

    if (!std::filesystem::exists(cache_file_directory)) {
        std::error_code ec;
        if (!std::filesystem::create_directories(cache_file_directory, ec) || ec) {
            spdlog::error("failed to create directory {}", cache_file_directory);
            return boost::outcome_v2::failure(static_cast<std::errc>(ec.value()));
        }
    }

    if (!std::filesystem::is_directory(cache_file_directory)) {
        spdlog::error("cache directory path {} is not a directory", cache_file_directory);
        return boost::outcome_v2::failure(std::errc::not_a_directory);
    }

    return engine::open_file(cache_file_path, mode);
}

engine::Result<utils::FileHandle> engine::create_cache_file(std::string_view name)
{
    return open_cache_file(name, "wb");
}

engine::Result<utils::FileHandle> engine::get_cache_file(std::string_view name, std::span<std::filesystem::path const> ref_files)
{
    if (ref_files.empty())
        return open_cache_file(name, "rb");

    auto const cache_file_path = s_cache_directory / name;

    std::error_code ec;
    auto cache_time = std::filesystem::last_write_time(cache_file_path, ec);
    if (ec) {
        spdlog::error("failed to obtain last write time for file {}", cache_file_path);
        return boost::outcome_v2::failure(static_cast<std::errc>(ec.value()));
    }

    auto const maybe_max_time = [&]() {
        std::optional<std::filesystem::file_time_type> maybe_time;

        for (auto const &ref_file : ref_files) {
            auto write_time = std::filesystem::last_write_time(ref_file, ec);

            if (ec) {
                spdlog::warn("failed to obtain last write time for {}, ignoring", ref_file);
                continue;
            }

            if (maybe_time) {
                maybe_time.emplace(std::max(*maybe_time, write_time));
            } else {
                maybe_time = std::move(write_time);
            }
        }

        return maybe_time;
    }();

    if (maybe_max_time.has_value() && maybe_max_time.value() < cache_time)
        return open_cache_file(name, "rb");

    return boost::outcome_v2::failure(std::errc::no_such_file_or_directory);
}