#ifndef MATH_BITS_HPP
#define MATH_BITS_HPP

#ifdef __has_include
#if __has_include(<bit>)
#define HAS_BIT_HEADER
#endif
#endif

#ifdef __has_cpp_attribute
#if __has_cpp_attribute(__cpp_lib_bitops)
#define HAS_LIB_BITOPTS
#endif
#endif

#if defined(HAS_BIT_INCLUDE) && defined(HAS_LIB_BITOPS)
#include <bits>
namespace math {
    template <typename T>
    constexpr int popcount(T x) noexcept
    {
        return std::popcount(x);
    }
    template <typename T>
    constexpr bool has_single_bit(T x) noexcept
    {
        return std::has_single_bit(x);
    }
} // namespace math
#else
#include <type_traits>
namespace math {
    template <typename T, std::enable_if_t<std::is_unsigned_v<T>, std::nullptr_t> = nullptr>
    constexpr int popcount(T x) noexcept
    {
        int count = 0;
        while (x) {
            if (x & 1) ++count;
            x = x >> 1;
        }
        return count;
    }
    template <typename T, std::enable_if_t<std::is_unsigned_v<T>, std::nullptr_t> = nullptr>
    constexpr bool has_single_bit(T x) noexcept
    {
        return x != 0 && (x & (x - 1)) == 0;
    }
} // namespace math
#endif

#ifdef HAS_BIT_HEADER
#undef HAS_BIT_HEADER
#endif

#ifdef HAS_LIB_BITOPS
#undef HAS_LIB_BITOPS
#endif

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
