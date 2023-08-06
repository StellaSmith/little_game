#ifndef ENGINE_SYSTEM_OPEN_FILE_HPP
#define ENGINE_SYSTEM_OPEN_FILE_HPP

#include <engine/nonnull.hpp>
#include <engine/result.hpp>

#include <cstdio> // std::FILE
#include <filesystem> // std::filesystem::path
#include <system_error> // std::errc

namespace engine::system {
    /**
     * opens an std::FILE using the given native path string
     */
    [[nodiscard]] engine::result<engine::nonnull<std::FILE>, std::errc> open_file(engine::nonnull<std::filesystem::path::value_type const> path, engine::nonnull<char const> mode) noexcept;

    /**
     * opens an std::FILE using the given std::filesystem::path
     */
    [[nodiscard]] inline engine::result<engine::nonnull<std::FILE>, std::errc> open_file(std::filesystem::path const &path, engine::nonnull<char const> mode) noexcept
    {
        return open_file(path.c_str(), mode);
    }
} // namespace engine::system

#endif