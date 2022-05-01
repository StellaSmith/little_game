#ifndef UTILS_UNIQUE_RESOURCE_HPP
#define UTILS_UNIQUE_RESOURCE_HPP

#include <concepts>
#include <functional>
#include <type_traits>

namespace utils {

    template <typename T, typename Deleter>
    class unique_resource : private Deleter {
    public:
        using resource_type = T;
        using deleter_type = std::remove_cv_t<Deleter>;

    private:
        static constexpr decltype(auto) invalid() noexcept
        {
            if constexpr (requires { { deleter_type::invalid } -> std::equality_comparable_with<resource_type> ; }) {
                return resource_type::invalid;
            } else if constexpr (requires(deleter_type & deleter) { { deleter.invalid() } -> std::equality_comparable_with<resource_type> ; }) {
                return Deleter::invalid();
            }
        }

    public:
        unique_resource() noexcept requires std::is_default_constructible_v<deleter_type>
            : unique_resource(nullptr)
        {
        }

        unique_resource(std::nullptr_t) noexcept requires std::is_default_constructible_v<deleter_type>
            : unique_resource(invalid())
        {
        }

        explicit unique_resource(resource_type resource) noexcept requires std::is_default_constructible_v<deleter_type>
            : Deleter(),
              m_resource(resource)
        {
        }

        unique_resource(resource_type resource, deleter_type const &deleter) noexcept
            : Deleter(deleter)
            , m_resource(resource)
        {
        }

        unique_resource(resource_type resource, deleter_type &&deleter) noexcept
            : Deleter(std::move(deleter))
            , m_resource(resource)
        {
        }

        unique_resource(unique_resource &&other) noexcept
            : Deleter(std::move(other.get_deleter()))
            , m_resource(other.m_resource)
        {
            other.invalidate();
        }

    private:
        constexpr void invalidate() noexcept
        {
            m_resource = invalid();
        }

    public:
        Deleter const &get_deleter() const noexcept
        {
            return *static_cast<Deleter const *>(this);
        }

        Deleter &get_deleter() noexcept
        {
            return *static_cast<Deleter const *>(this);
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return m_resource == invalid();
        }

        void reset(resource_type resource) noexcept
        {
            if (*this)
                std::invoke(get_deleter(), m_resource);
            m_resource = resource;
        }

        void reset(std::nullptr_t = nullptr) noexcept
        {
            if (*this)
                std::invoke(get_deleter(), m_resource);
            invalidate();
        }

        [[nodiscard]] resource_type release() noexcept
        {
            auto copy = m_resource;
            invalidate();
            return copy;
        }

        void swap(unique_resource &other) noexcept
        {
            using std::swap;
            swap(m_resource, other.m_resource);
            swap(get_deleter(), other.get_deleter());
        }

        ~unique_resource()
        {
            reset();
        }

    private:
        resource_type m_resource;
    };
}

#endif