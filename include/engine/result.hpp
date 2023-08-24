#ifndef ENGINE_RESULT_HPP
#define ENGINE_RESULT_HPP

#include <boost/outcome/basic_result.hpp>
#include <boost/outcome/policy/all_narrow.hpp>
#include <boost/outcome/try.hpp>

#define TRY BOOST_OUTCOME_TRYX
#define MUST(expr) ([](auto const &location) {                      \
    if (auto _result = (expr); !_result.has_value()) [[unlikely]] { \
        using std::make_error_code;                                 \
        spdlog::default_logger_raw()->log(                          \
            location,                                               \
            spdlog::level::critical,                                \
            "expr {:?} failed: {}",                                 \
            #expr, make_error_code(_result.error()));               \
        std::terminate();                                           \
    } else {                                                        \
        return std::move(_result).value();                          \
    }                                                               \
}(spdlog::source_loc { __FILE__, __LINE__, SPDLOG_FUNCTION }))

namespace engine {
    template <typename Value, typename Error>
    using result = boost::outcome_v2::basic_result<Value, Error, boost::outcome_v2::policy::all_narrow>;
}

#endif