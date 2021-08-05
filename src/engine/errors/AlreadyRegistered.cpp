#include <engine/errors/AlreadyRegistered.hpp>

engine::errors::AlreadyRegistered::AlreadyRegistered() noexcept
    : exception()
    , m_message("engine::errors::AlreadyRegistered")
{
}

engine::errors::AlreadyRegistered::AlreadyRegistered(char const *message) noexcept
    : exception()
    , m_message(message)
{
}

char const *engine::errors::AlreadyRegistered::what() const noexcept
{
    return m_message;
}