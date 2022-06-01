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

#endif