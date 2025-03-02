#ifndef ENGINE_NONNULL_HPP
#define ENGINE_NONNULL_HPP

#include <cassert>
#include <fmt/base.h>

namespace engine {
#ifndef NDEBUG

    template <typename T>
    using nonnull = T *;

#else

    template <typename T>
    struct nonnull {
    public:
        nonnull(T *ptr) noexcept
            : ptr(ptr)
        {
            assert(ptr != nullptr);
        }

        [[gnu::always_inline]]
        operator T *() const noexcept
        {
            return ptr;
        }

        [[gnu::always_inline]]
        T *operator->() const noexcept
        {
            return ptr;
        }

        [[gnu::always_inline]]
        T &operator*() const noexcept
        {
            return *ptr;
        }

    private:
        T *ptr;
    };

#endif
} // namespace engine

#if defined(NDEBUG)
template <>
class fmt::formatter<engine::nonnull<char const>> : public fmt::formatter<char const *> { };

template <>
class fmt::formatter<engine::nonnull<wchar_t const>> : public fmt::formatter<wchar_t const *> { };
#endif

#endif
