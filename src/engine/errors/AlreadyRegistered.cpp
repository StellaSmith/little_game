#include <engine/errors/AlreadyRegistered.hpp>

engine::errors::AlreadyRegistered::AlreadyRegistered() noexcept
    : std::exception()
{
}

char const *engine::errors::AlreadyRegistered::what() const noexcept
{
    return "engine::errors::AlreadyRegistered";
}