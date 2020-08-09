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
}
#else
#include <type_traits>
namespace math {
    template <typename T>
    constexpr auto popcount(T x) -> std::enable_if_t<std::is_unsigned_v<T>, int> noexcept
    {
        int count = 0;
        while (x) {
            if (x & 1) ++count;
            x = x >> 1;
        }
        return count;
    }
    template <typename T>
    constexpr auto has_single_bit(T x) -> std::enable_if_t<std::is_unsigned_v<T>, bool> noexcept
    {
        return x != 0 && (x & (x - 1)) == 0;
    }
}
#endif

#ifdef HAS_BIT_HEADER
#undef HAS_BIT_HEADER
#endif

#ifdef HAS_LIB_BITOPS
#undef HAS_LIB_BITOPS
#endif

#endif
