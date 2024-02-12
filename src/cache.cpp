#include <engine/Config.hpp>
#include <engine/cache.hpp>
#include <engine/resources.hpp>

#include <boost/outcome/success_failure.hpp>
#include <fmt/std.h>
#include <spdlog/spdlog.h>

#include <algorithm> // std::max
#include <cstdlib> // std::getenv
#include <optional>
#include <vector>

static engine::result<engine::File, std::errc> open_cache_file(std::string_view name, engine::nonnull<char const> mode)
{
    while (!name.empty() && name.back() == '/')
        name.remove_suffix(1);

    if (name.empty())
        return boost::outcome_v2::failure(std::errc::invalid_argument);

    auto const &cache_directory = engine::config().folders.cache;
    auto const cache_file_path = cache_directory / name;
    auto const cache_file_directory = cache_file_path.parent_path();

    if (cache_file_directory.lexically_relative(cache_directory).string().starts_with("..")) {
        SPDLOG_ERROR("tried to open a cache file outside the cache directory: {} resolved to {}", name, cache_file_path);
        return boost::outcome_v2::failure(std::errc::permission_denied);
    }

    if (!std::filesystem::exists(cache_file_directory)) {
        std::error_code ec;
        if (!std::filesystem::create_directories(cache_file_directory, ec) || ec) {
            SPDLOG_ERROR("failed to create directory {}", cache_file_directory);
            return boost::outcome_v2::failure(static_cast<std::errc>(ec.value()));
        } else {
            SPDLOG_INFO("created directory {}", cache_file_directory);
        }
    }

    if (!std::filesystem::is_directory(cache_file_directory)) {
        SPDLOG_ERROR("cache directory path {} is not a directory", cache_file_directory);
        return boost::outcome_v2::failure(std::errc::not_a_directory);
    }

    return engine::File::open(cache_file_path, mode);
}

engine::result<engine::File, std::errc> engine::create_cache_file(std::string_view name)
{
    auto cache_file = open_cache_file(name, "wb");
    if (cache_file)
        SPDLOG_INFO("created cache file {:?}", name);
    return cache_file;
}

engine::result<engine::File, std::errc> engine::get_cache_file(std::string_view name, std::span<std::filesystem::path const> ref_files)
{
    if (ref_files.empty())
        return open_cache_file(name, "rb");

    auto const &cache_directory = engine::config().folders.cache;
    auto const cache_file_path = cache_directory / name;

    std::error_code ec;
    auto cache_time = std::filesystem::last_write_time(cache_file_path, ec);
    if (ec) {
        SPDLOG_ERROR("failed to obtain last write time for file {}: {}", cache_file_path, ec.message());
        return boost::outcome_v2::failure(static_cast<std::errc>(ec.value()));
    }

    auto const maybe_max_time = [&]() {
        std::optional<std::filesystem::file_time_type> maybe_time;

        for (auto const &ref_file : ref_files) {
            auto write_time = std::filesystem::last_write_time(ref_file, ec);

            if (ec) {
                SPDLOG_WARN("failed to obtain last write time for {}, ignoring: {}", ref_file, ec.message());
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