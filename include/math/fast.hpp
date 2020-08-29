#ifndef MATH_FAST_HPP
#define MATH_FAST_HPP

#include <array>
#include <cmath>
#include <glm/ext/scalar_constants.hpp>

namespace math {

    // functions that use lookup tables instead of calculating stuff every time

    extern std::array<double, 256> const sin_table_256;
    extern std::array<double, 8192> const sin_table_8192;

    inline double fast_sin256(double x) noexcept
    {
        constexpr double magic_number = sin_table_256.size() / glm::pi<double>() / 2.0;
        return sin_table_256[(unsigned)(x * magic_number) & (sin_table_256.size() - 1)];
    }

    inline double fast_sin8192(double x) noexcept
    {
        constexpr double magic_number = sin_table_8192.size() / glm::pi<double>() / 2.0;
        return sin_table_8192[(unsigned)(x * magic_number) & (sin_table_8192.size() - 1)];
    }

    inline double fast_cos256(double x) noexcept
    {
        return fast_sin256(x + glm::pi<double>() / 2.0);
    }

    inline double fast_cos8192(double x) noexcept
    {
        return fast_sin8192(x + glm::pi<double>() / 2.0);
    }

} // namespace math

#endif
