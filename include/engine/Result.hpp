#ifndef ENGINE_RESULT_HPP
#define ENGINE_RESULT_HPP

#include <system_error> // std::errc

#include <boost/outcome/std_result.hpp>

namespace engine {
    template <typename Value, typename Error = std::errc>
    using Result = boost::outcome_v2::std_result<Value, Error>;
}

#endif