#include <engine/system/page_size.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

std::size_t engine::system::page_size() noexcept
{
    static std::size_t const _value = []() {
        SYSTEM_INFO system_info;
        GetNativeSystemInfo(&system_info);
        return system_info.dwPageSize;
    }();

    return _value;
}
