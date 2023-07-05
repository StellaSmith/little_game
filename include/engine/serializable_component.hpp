#ifndef SERIALIZABLE_COMPONENT_HPP
#define SERIALIZABLE_COMPONENT_HPP

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_free.hpp>

#define SERIALIZE_COMPONENT_IMPL(r, op, member) archive op boost::serialization::make_nvp(BOOST_PP_STRINGIZE(member), component.member);

#define SERIALIZABLE_COMPONENT(T, ...)                                                                             \
    namespace boost::serialization {                                                                               \
        template <class Archive>                                                                                   \
        void save([[maybe_unused]] Archive &archive, [[maybe_unused]] T const &component, unsigned int const)      \
        {                                                                                                          \
            __VA_OPT__(BOOST_PP_SEQ_FOR_EACH(SERIALIZE_COMPONENT_IMPL, <<, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))) \
        }                                                                                                          \
                                                                                                                   \
        template <class Archive>                                                                                   \
        void load([[maybe_unused]] Archive &archive, [[maybe_unused]] T &component, unsigned int const)            \
        {                                                                                                          \
            __VA_OPT__(BOOST_PP_SEQ_FOR_EACH(SERIALIZE_COMPONENT_IMPL, >>, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))) \
        }                                                                                                          \
    }                                                                                                              \
    BOOST_SERIALIZATION_SPLIT_FREE(T)

#endif