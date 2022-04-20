#ifndef ENGINE_ERRORS_ALREADY_REGISTERED_HPP
#define ENGINE_ERRORS_ALREADY_REGISTERED_HPP

#include <exception>

namespace engine::errors {

    class AlreadyRegistered : public std::exception {

    public:
        explicit AlreadyRegistered() noexcept;

        char const *what() const noexcept override;
    };

}

#endif