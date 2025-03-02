#ifndef ENGINE_SYSTEM_OPEN_FILE_HPP
#define ENGINE_SYSTEM_OPEN_FILE_HPP

#include <engine/nonnull.hpp>
#include <engine/result.hpp>

#include <cstdio> // std::FILE
#include <filesystem> // std::filesystem::path

namespace engine::system {
    /**
     * opens an std::FILE using the given native path string
     */
    [[nodiscard]]
    engine::nonnull<std::FILE> fopen(engine::nonnull<std::filesystem::path::value_type const> path, engine::nonnull<char const> mode);

    /**
     * opens an std::FILE using the given std::filesystem::path
     */
    [[nodiscard]]
    inline engine::nonnull<std::FILE> fopen(std::filesystem::path const &path, engine::nonnull<char const> mode)
    {
        // because nonnull & std::filesystem::path are both convertible from char const*,
        // we need to explictly use the right type
        return engine::system::fopen(nonnull<std::filesystem::path::value_type const>(path.c_str()), mode);
    }
} // namespace engine::system

#endif
