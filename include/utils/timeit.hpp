#ifndef UTILS_TIMEIT_HPP
#define UTILS_TIMEIT_HPP

#include <spdlog/spdlog.h>

#include <chrono>
#include <string_view>

namespace utils {
    struct TimeIt {
        using clock_type = std::conditional_t<std::chrono::high_resolution_clock::is_steady, std::chrono::high_resolution_clock, std::chrono::steady_clock>;

        TimeIt(std::string_view name)
            : m_start { clock_type::now() }
            , m_name { name }
        {
        }

        ~TimeIt()
        {
            auto const now = clock_type::now();
            auto const delta = static_cast<std::chrono::duration<double, std::milli>>(now - m_start);
            spdlog::info("TIME: {} took {}ms", m_name, delta.count());
        }

        clock_type::time_point m_start;
        std::string_view m_name;
    };
} // namespace utils

#endif