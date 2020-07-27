#ifndef OPERATORS_HPP
#define OPERATORS_HPP

#include <cstddef>
#include <cstdint>

constexpr auto operator""_i8(unsigned long long n) noexcept
{
    return static_cast<std::int8_t>(n);
}

constexpr auto operator""_u8(unsigned long long n) noexcept
{
    return static_cast<std::uint8_t>(n);
}

constexpr auto operator""_i16(unsigned long long n) noexcept
{
    return static_cast<std::int16_t>(n);
}

constexpr auto operator""_u16(unsigned long long n) noexcept
{
    return static_cast<std::uint16_t>(n);
}

constexpr auto operator""_i32(unsigned long long n) noexcept
{
    return static_cast<std::int32_t>(n);
}

constexpr auto operator""_u32(unsigned long long n) noexcept
{
    return static_cast<std::uint32_t>(n);
}

constexpr auto operator""_i64(unsigned long long n) noexcept
{
    return static_cast<std::int64_t>(n);
}

constexpr auto operator""_u64(unsigned long long n) noexcept
{
    return static_cast<std::uint64_t>(n);
}

constexpr auto operator""_sz(unsigned long long n) noexcept
{
    return static_cast<std::size_t>(n);
}

#endif