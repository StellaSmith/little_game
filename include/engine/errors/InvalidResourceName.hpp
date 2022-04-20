#ifndef ENGINE_ERRORS_INVALID_RESOURCE_NAME_HPP
#define ENGINE_ERRORS_INVALID_RESOURCE_NAME_HPP

#include <exception>
#include <string>

namespace engine::errors {

    class InvalidResourceName : public std::exception {

    public:
        explicit InvalidResourceName(std::string m_resource_name) noexcept;
        std::string const &resource_name() const noexcept;

        char const *what() const noexcept override;

    private:
        std::string m_resource_name;
    };

}

#endif