#include <engine/BlockRegistry.hpp>

engine::AlreadyRegistered::AlreadyRegistered() noexcept
    : exception()
    , m_message("Already registered")
{
}

engine::AlreadyRegistered::AlreadyRegistered(char const *message) noexcept
    : exception()
    , m_message(message)
{
}

char const *engine::AlreadyRegistered::what() const noexcept
{
    return m_message;
}

engine::BlockType *engine::BlockRegistry::Get(std::string_view name)
{
    if (auto it = find(name); it != end())
        return (*it).second;
    else
        return nullptr;
}

engine::BlockRegistry::iterator engine::BlockRegistry::find(std::string_view name) noexcept
{
    auto it = std::lower_bound(m_registeredBlocks.begin(), m_registeredBlocks.end(), name, [](Item const &item, std::string_view name) {
        return item.name < name;
    });
    if (it == m_registeredBlocks.end() || it->name != name)
        return end();
    return iterator(&*it);
}

engine::BlockRegistry::const_iterator engine::BlockRegistry::find(std::string_view name) const noexcept
{
    auto it = std::lower_bound(m_registeredBlocks.begin(), m_registeredBlocks.end(), name, [](Item const &item, std::string_view name) {
        return item.name < name;
    });
    if (it == m_registeredBlocks.end() || it->name != name)
        return end();
    return const_iterator(&*it);
}