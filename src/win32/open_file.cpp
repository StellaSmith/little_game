#include <engine/system/open_file.hpp>

#include <cstring> // std::strlen
#include <stdio.h> // _wfopen_s

engine::result<engine::nonnull<std::FILE>, std::errc> engine::system::open_file(std::filesystem::path::value_type const *path, char const *mode) noexcept
{
    // widen the mode string to wchar_t
    wchar_t buf[32];
    std::size_t const size = std::strlen(mode);
    if (size >= std::size(buf))
        return std::errc::argument_list_too_long;
    *std::copy(mode, mode + size, buf) = L'\0';

    std::FILE *fp = nullptr;
    int const error = ::_wfopen_s(&fp, path, &buf[0]);

    if (error)
        return static_cast<std::errc>(error);
    else
        return fp;
}