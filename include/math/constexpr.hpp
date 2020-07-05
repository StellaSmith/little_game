#ifndef MATH_CONSTEXPR_HPP
#define MATH_CONSTRXPR_HPP

namespace math
{
    // constexpr integer power
    template <std::int64_t v, std::uint64_t x>
    struct c_ipow
    {
        constexpr static std::int64_t value = v * c_ipow<v, x - 1>::value;
    };

    template <std::int64_t v>
    struct c_ipow<v, 1>
    {
        constexpr static std::int64_t value = v;
    };

    template <std::int64_t v>
    struct c_ipow<v, 0>
    {
        constexpr static std::int64_t value = 1;
    };

    template <std::int64_t v, std::uint64_t x>
    inline constexpr std::int64_t c_ipow_v = c_ipow<v, x>::value;

    // constexpr integer logarithm
    template <std::int64_t v, std::uint64_t x>
    struct c_ilog
    {
    };

    template <std::int64_t v, std::uint64_t x>
    inline constexpr std::int64_t c_ilog_v = c_ilog<v, x>::value;

    template <std::int64_t v>
    inline constexpr std::int64_t c_ilog10_v = c_ilog<v, 10>::value;

    template <std::int64_t v>
    inline constexpr std::int64_t c_ilog2_v = c_ilog<v, 2>::value;

} // namespace math

#endif