#include <engine/errors/InvalidResourceName.hpp>

engine::errors::InvalidResourceName::InvalidResourceName(std::string resource_name) noexcept
    : std::exception()
    , m_resource_name(std::move(resource_name))
{
}

const char *engine::errors::InvalidResourceName::what() const noexcept
{
    return "engine::errors::InvalidResourceName";
}

std::string const &engine::errors::InvalidResourceName::resource_name() const noexcept
{
    return m_resource_name;
}