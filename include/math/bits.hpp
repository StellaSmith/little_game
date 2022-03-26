#ifndef HAS_LIB_BITOPS
#define HAS_LIB_BITOPS

#include <glm/glm.hpp>

#include <cstdint>

namespace math {
    constexpr std::uint32_t pack_u32(std::uint8_t x, std::uint8_t y, std::uint8_t z, std::uint8_t w = 0) noexcept
    {
        return (x << 24) | (y << 16) | (z << 8) | w;
    }

    constexpr glm::u8vec4 unpack_u32(std::uint64_t v) noexcept
    {
        return { (v >> 24) & 0xFF, (v >> 16 & 0xFF), (v >> 8 & 0xFF), v & 0xFF };
    }
} // namespace math

#endif
