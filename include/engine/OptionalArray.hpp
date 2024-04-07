#ifndef ENGINE_OPTIONAL_ARRAY_HPP
#define ENGINE_OPTIONAL_ARRAY_HPP

#include <bitset>
#include <climits>
#include <cstddef>
#include <utility>

namespace engine {

    template <typename T, std::size_t N>
    struct OptionalArray {
        [[nodiscard]]
        constexpr std::size_t capacity() const noexcept
        {
            return N;
        }

        [[nodiscard]]
        constexpr std::size_t size() const noexcept
        {
            return m_set.size();
        }

        [[nodiscard]]
        constexpr bool has(std::size_t index) const noexcept
        {
            return m_set.test(index);
        }

        [[nodiscard]]
        constexpr T *at(std::size_t index) noexcept
        {
            if (has(index))
                return ptr(index);
            else
                return nullptr;
        }

        [[nodiscard]]
        constexpr T const *at(std::size_t index) const noexcept
        {
            if (has(index))
                return ptr(index);
            else
                return nullptr;
        }

        [[nodiscard]]
        constexpr T &
        operator[](std::size_t index) noexcept
        {
            assert(has(index));
            return *ptr(index);
        }

        [[nodiscard]]
        constexpr T const &
        operator[](std::size_t index) const noexcept
        {
            assert(has(index));
            return *ptr(index);
        }

        template <typename... Args>
        T &emplace(std::size_t index, Args &&...args)
        {
            if (!has(index)) {
                new (ptr(index)) T(std::forward<Args>(args)...);
                m_set.set(index);
            }
            return *ptr(index);
        }

        void erase(std::size_t index) noexcept
        {
            if (has(index)) {
                ptr(index)->~T();
                m_set.reset(index);
            }
        }

        ~OptionalArray() noexcept
        {
            for (std::size_t i = 0; i < N; ++i) {
                if (has(i))
                    ptr(i)->~T();
            }
        }

    private:
        [[nodiscard]]
        T *ptr(std::size_t index) noexcept
        {
            return reinterpret_cast<T *>(m_storage + index * sizeof(T));
        }

        [[nodiscard]]
        T const *ptr(std::size_t index) const noexcept
        {
            return reinterpret_cast<T const *>(m_storage + index * sizeof(T));
        }

        alignas(T) char m_storage[N * sizeof(T)];
        std::bitset<N> m_set = {};
    };

} // namespace engine

#endif