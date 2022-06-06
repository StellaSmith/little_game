#ifndef UTILS_COMPRESSED_PAIR_HPP
#define UTILS_COMPRESSED_PAIR_HPP

#include <type_traits>

namespace utils {

    template <typename T, typename U>
    struct compressed_pair;

    template <typename T, typename U>
    requires(!std::is_empty_v<T> && !std::is_empty_v<U>) struct compressed_pair<T, U> {
        using first_type = T;
        using second_type = U;
        T first;
        U second;
    };

    template <typename T, typename U>
    requires(std::is_empty_v<T> && !std::is_empty_v<U>) struct compressed_pair<T, U> {
        using first_type = T;
        using second_type = U;
        U second;
    };

    template <typename T, typename U>
    requires(!std::is_empty_v<T> && std::is_empty_v<U>) struct compressed_pair<T, U> {
        using first_type = T;
        using second_type = U;
        T first;
    };

    template <typename T, typename U>
    requires(std::is_empty_v<T> &&std::is_empty_v<U>) struct compressed_pair<T, U> {
        using first_type = T;
        using second_type = U;
    };
}

#include <utility>

namespace std {
    template <typename First, typename Second>
    struct tuple_element<0, utils::compressed_pair<First, Second>> {
        using type = First;
    };

    template <typename First, typename Second>
    struct tuple_element<1, utils::compressed_pair<First, Second>> {
        using type = Second;
    };

    template <typename First, typename Second>
    struct tuple_size<utils::compressed_pair<First, Second>> : std::integral_constant<std::size_t, 2> {
    };
}

namespace utils {
    namespace impl {

        template <typename T>
        struct is_compressed_pair : std::false_type {
        };

        template <typename First, typename Second>
        struct is_compressed_pair<utils::compressed_pair<First, Second>> : std::true_type {
        };

    }

    template <std::size_t I, typename T>
    requires(
        (impl::is_compressed_pair<std::remove_cvref_t<T>>::value)
        && I == 0
        && !std::is_empty_v<typename T::first_type>)

        decltype(auto) get(T &&pair) noexcept
    {
        return static_cast<T &&>(pair).first;
    }

    template <std::size_t I, typename T>
    requires(
        (impl::is_compressed_pair<std::remove_cvref_t<T>>::value)
        && I == 1
        && !std::is_empty_v<typename T::second_type>)

        decltype(auto) get(T &&pair) noexcept
    {
        return static_cast<T &&>(pair).second;
    }
}

#endif