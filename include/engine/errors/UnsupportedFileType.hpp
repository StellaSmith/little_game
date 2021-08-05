#ifndef ENGINE_ERRORS_UNSUPPORTED_FILE_TYPE_HPP
#define ENGINE_ERRORS_UNSUPPORTED_FILE_TYPE_HPP

#include <exception>

namespace engine::errors {

    class UnsupportedFileType : public std::exception {

    public:
        explicit UnsupportedFileType() noexcept;
        explicit UnsupportedFileType(char const *message) noexcept;

        char const *what() const noexcept override;

    private:
        char const *m_message;
    };

}

#endif