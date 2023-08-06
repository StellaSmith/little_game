#include <engine/system/block_size.hpp>

engine::result<std::size_t, std::errc> engine::system::block_size(engine::nonnull<std::FILE>) noexcept
{
    // TODO: how?
    return std::errc::not_supported;
}