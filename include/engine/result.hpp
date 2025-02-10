#ifndef ENGINE_RESULT_HPP
#define ENGINE_RESULT_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#include <spdlog/spdlog.h>
#pragma GCC diagnostic pop

#include <boost/outcome/basic_result.hpp>
#include <boost/outcome/policy/all_narrow.hpp>
#include <boost/outcome/try.hpp>

/**
 * propagates on error
 */
#define TRY BOOST_OUTCOME_TRYX
/**
 * terminates on error
 */
#define MUST(expr)                                         \
    ({                                                     \
        auto _result = (expr);                             \
        if (!_result.has_value()) [[unlikely]] {           \
            using std::make_error_code;                    \
            auto _code = make_error_code(_result.error()); \
            SPDLOG_CRITICAL("{:?} failed: {}",             \
                #expr,                                     \
                _code.message());                          \
            std::terminate();                              \
        }                                                  \
        std::move(_result).value();                        \
    })
/**
 * throws on error
 */
#define EXPECT(expr)                                       \
    ({                                                     \
        auto _result = (expr);                             \
        if (!_result.has_value()) [[unlikely]] {           \
            using std::make_error_code;                    \
            auto _code = make_error_code(_result.error()); \
            SPDLOG_ERROR("{:?} failed: {}",                \
                #expr,                                     \
                _code.message());                          \
            throw std::system_error(                       \
                std::move(_code),                          \
                #expr " failed");                          \
        }                                                  \
        std::move(_result).value();                        \
    })

namespace engine {
    // policy dictates behaviour when accessing the value on error
    // the all_narrow policy makes it UB to access the value on error (no checks are done)
    /**
     * enum of value or error
     * used for cases where an error should be handled gracefully
     */
    template <typename Value, typename Error>
    using result = boost::outcome_v2::basic_result<
        Value,
        Error,
        boost::outcome_v2::policy::all_narrow>;
}

#endif