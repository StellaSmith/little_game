#pragma once

#include <glad/glad.h>

#include <functional>
#include <limits>
#include <tuple>
#include <utility>

namespace engine::rendering::opengl {
    template <auto Bind, typename... Args>
    class BoundContext {
    public:
        static constexpr GLuint invalid_name = std::numeric_limits<GLuint>::max();
        static constexpr auto bind_function = Bind;

        explicit BoundContext(GLuint name, Args... args)
            : m_name(name)
            , m_arguments(args...)

        {
            std::apply(*bind_function, std::tuple_cat(m_arguments, std::make_tuple(m_name)));
        }

        BoundContext(BoundContext const &) = delete;
        BoundContext(BoundContext &&other) noexcept
            : m_name(std::exchange(other.m_name, invalid_name))
            , m_arguments(std::move(other.m_arguments))
        {
        }

        BoundContext &operator=(BoundContext const &) = delete;
        BoundContext &operator=(BoundContext &&other) noexcept
        {
            this->m_name = std::exchange(other.m_name, invalid_name);
            this->m_arguments = std::move(other.m_arguments);
            return *this;
        }

        GLuint name() { return m_name; }

    protected:
        auto const &arguments() { return m_arguments; }

    public:
        ~BoundContext()
        {
            std::apply(*bind_function, std::tuple_cat(m_arguments, std::make_tuple(0)));
        }

    private:
        GLuint m_name;
        std::tuple<Args...> m_arguments;
    };

    template <typename T, typename BindContext, auto Create, auto Delete>
    class Object {
    public:
        static constexpr GLuint invalid_name = std::numeric_limits<GLuint>::max();
        static constexpr auto create_function = Create;
        static constexpr auto delete_function = Delete;

        explicit Object()
            : m_name(invalid_name)
        {
        }

        explicit Object(GLuint name)
            : m_name(name)
        {
        }

        Object(Object &&other)
            : m_name(std::exchange(other.m_name, invalid_name))
        {
        }

        T &create() &
        {
            std::invoke(*create_function, 1, &this->m_name);
            return static_cast<T &>(*this);
        }

        T create() &&
        {
            std::invoke(*create_function, 1, &this->m_name);
            return static_cast<T>(std::move(*this));
        }

        template <typename... Args>
        BindContext bind(Args... args)
        {
            return BindContext(m_name, args...);
        }

        GLuint name()
        {
            return this->m_name;
        }

        ~Object()
        {
            if (this->m_name != invalid_name)
                std::invoke(*delete_function, 1, &this->m_name);
        }

    private:
        GLuint m_name;
    };
} // namespace engine::rendering::opengl
