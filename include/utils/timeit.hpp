#ifndef UTILS_TIMEIT_HPP
#define UTILS_TIMEIT_HPP

#include <chrono>

namespace utils {
    struct TimeIt {
        using clock_type = std::conditional_t<std::chrono::high_resolution_clock::is_steady, std::chrono::high_resolution_clock, std::chrono::steady_clock>;
    };
} // namespace utils

#endif