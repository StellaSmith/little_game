#include <engine/errors/UnsupportedFileType.hpp>

engine::errors::UnsupportedFileType::UnsupportedFileType() noexcept
    : std::exception()

{
}

char const *engine::errors::UnsupportedFileType::what() const noexcept
{
    return "engine::errors::UnsupportedFileType";
}