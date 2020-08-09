#ifndef MATH_FAST_HPP
#define MATH_FAST_HPP

#include <array>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace math {

    // functions that use lookup tables instead of calculating stuff every time

    extern std::array<double, 256> const sin_table_256;
    extern std::array<double, 8192> const sin_table_8192;

    inline double fast_sin256(double x) noexcept
    {
        constexpr double magic_number = 256.0 / M_PI / 2.0;
        return sin_table_256[(int)(x * magic_number) & 255];
    }

    inline double fast_sin8192(double x) noexcept
    {
        constexpr double magic_number = 8192.0 / M_PI / 2.0;
        return sin_table_8192[(int)(x * magic_number) & 8191];
    }

    inline double fast_cos256(double x) noexcept
    {
        return fast_sin256(x + M_PI / 2.0);
    }

    inline double fast_cos8192(double x) noexcept
    {
        return fast_sin8192(x + M_PI / 2.0);
    }

} // namespace math

#endif
