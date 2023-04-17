#include <engine/Config.hpp>
#include <engine/Stream.hpp>
#include <engine/cache.hpp>

#include <boost/outcome/success_failure.hpp>
#include <fmt/std.h>
#include <spdlog/spdlog.h>

#include <algorithm> // std::max
#include <cstdlib> // std::getenv
#include <optional>
#include <vector>

static engine::Result<utils::FileHandle> open_cache_file(std::string_view name, char const *mode)
{
    while (!name.empty() && name.back() == '/')
        name.remove_suffix(1);

    if (name.empty())
        return boost::outcome_v2::failure(std::errc::invalid_argument);

    auto const &cache_directory = engine::config().folders.cache;
    auto const cache_file_path = cache_directory / name;
    auto const cache_file_directory = cache_file_path.parent_path();

    if (!cache_file_directory.lexically_relative(cache_directory).native().starts_with(cache_directory.native())) {
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

    auto const &cache_directory = engine::config().folders.cache;
    auto const cache_file_path = cache_directory / name;

    std::error_code ec;
    auto cache_time = std::filesystem::last_write_time(cache_file_path, ec);
    if (ec) {
        spdlog::error("failed to obtain last write time for file {}: {}", cache_file_path, ec.message());
        return boost::outcome_v2::failure(static_cast<std::errc>(ec.value()));
    }

    auto const maybe_max_time = [&]() {
        std::optional<std::filesystem::file_time_type> maybe_time;

        for (auto const &ref_file : ref_files) {
            auto write_time = std::filesystem::last_write_time(ref_file, ec);

            if (ec) {
                spdlog::warn("failed to obtain last write time for {}, ignoring: {}", ref_file, ec.message());
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