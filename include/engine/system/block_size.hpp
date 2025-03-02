#ifndef ENGINE_SYSTEM_BLOCK_SIZE_HPP
#define ENGINE_SYSTEM_BLOCK_SIZE_HPP

#include <engine/nonnull.hpp>
#include <engine/result.hpp>

#include <cstdio>

namespace engine::system {
    [[nodiscard]] std::size_t block_size(engine::nonnull<std::FILE>);
} // namespace engine::system

#endif
