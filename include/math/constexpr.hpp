#ifndef MATH_CONSTEXPR_HPP
#define MATH_CONSTEXPR_HPP

namespace math {
    // constexpr integer power
    template <std::int64_t v, std::uint64_t x>
    struct c_ipow {
        constexpr static std::int64_t value = v * c_ipow<v, x - 1>::value;
    };

    template <std::int64_t v>
    struct c_ipow<v, 1> {
        constexpr static std::int64_t value = v;
    };

    template <std::int64_t v>
    struct c_ipow<v, 0> {
        constexpr static std::int64_t value = 1;
    };

    template <std::int64_t v, std::uint64_t x>
    inline constexpr std::int64_t c_ipow_v = c_ipow<v, x>::value;

    // constexpr integer logarithm (rounded down)
    template <std::uint64_t x>
    struct c_ilog10 {
        constexpr static std::uint64_t value = 1 + c_ilog10<x / 10>::value;
    };

    template <>
    struct c_ilog10<9> {
        constexpr static std::uint64_t value = 1;
    };
    template <>
    struct c_ilog10<8> {
        constexpr static std::uint64_t value = 1;
    };
    template <>
    struct c_ilog10<7> {
        constexpr static std::uint64_t value = 1;
    };
    template <>
    struct c_ilog10<6> {
        constexpr static std::uint64_t value = 1;
    };
    template <>
    struct c_ilog10<5> {
        constexpr static std::uint64_t value = 1;
    };
    template <>
    struct c_ilog10<4> {
        constexpr static std::uint64_t value = 1;
    };
    template <>
    struct c_ilog10<3> {
        constexpr static std::uint64_t value = 1;
    };
    template <>
    struct c_ilog10<2> {
        constexpr static std::uint64_t value = 1;
    };

    template <>
    struct c_ilog10<1> {
        constexpr static std::uint64_t value = 0;
    };

    template <>
    struct c_ilog10<0> {
    };

    template <std::uint64_t x>
    struct c_ilog2 {
        constexpr static std::uint64_t value = 1 + c_ilog2<x / 2>::value;
    };

    template <>
    struct c_ilog2<1> {
        constexpr static std::uint64_t value = 0;
    };

    template <>
    struct c_ilog2<0> {
    };

    template <std::uint64_t x>
    inline constexpr std::uint64_t c_ilog10_v = c_ilog10<x>::value;

    template <std::uint64_t x>
    inline constexpr std::uint64_t c_ilog2_v = c_ilog2<x>::value;

} // namespace math

#endif