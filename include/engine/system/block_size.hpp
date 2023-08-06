#ifndef ENGINE_SYSTEM_BLOCK_SIZE_HPP
#define ENGINE_SYSTEM_BLOCK_SIZE_HPP

#include <engine/nonnull.hpp>
#include <engine/result.hpp>

#include <cstdio>
#include <system_error>

namespace engine::system {
    [[nodiscard]] engine::result<std::size_t, std::errc> block_size(engine::nonnull<std::FILE>) noexcept;
} // namespace engine::system

#endif