#include <engine/system/page_size.hpp>

#include <unistd.h>

std::size_t engine::system::page_size() noexcept
{
    static std::size_t const _value = sysconf(_SC_PAGE_SIZE);

    return _value;
}
