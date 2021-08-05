#include <engine/errors/UnsupportedFileType.hpp>

engine::errors::UnsupportedFileType::UnsupportedFileType() noexcept
    : exception()
    , m_message("engine::errors::UnsupportedFileType")
{
}

engine::errors::UnsupportedFileType::UnsupportedFileType(char const *message) noexcept
    : exception()
    , m_message(message)
{
}

char const *engine::errors::UnsupportedFileType::what() const noexcept
{
    return m_message;
}