#include <math/fast.hpp>

template <typename std::size_t N>
static std::array<double, N> calculate_sin_array()
{
    std::array<double, N> result;
    for (std::size_t i = 0; i < N; ++i)
        result[i] = std::sin(i * glm::pi<double>() * 2.0 / N);
    return result;
}

std::array<double, 256> const math::sin_table_256 = calculate_sin_array<256>();
std::array<double, 8192> const math::sin_table_8192 = calculate_sin_array<8192>();