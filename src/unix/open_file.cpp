#include <engine/system/open_file.hpp>

engine::result<engine::nonnull<std::FILE>, std::errc> engine::system::open_file(std::filesystem::path::value_type const *path, char const *mode) noexcept
{
    std::FILE *fp = std::fopen(path, mode);

    if (!fp)
        return static_cast<std::errc>(errno);
    else
        return fp;
}