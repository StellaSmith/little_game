#ifndef ENGINE_RESULT_HPP
#define ENGINE_RESULT_HPP

#include <boost/outcome/basic_result.hpp>
#include <boost/outcome/policy/all_narrow.hpp>

namespace engine {
    template <typename Value, typename Error>
    using result = boost::outcome_v2::basic_result<Value, Error, boost::outcome_v2::policy::all_narrow>;
}

#endif