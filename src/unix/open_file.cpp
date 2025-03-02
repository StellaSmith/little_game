#include <engine/nonnull.hpp>
#include <engine/system/open_file.hpp>
#include <system_error>

engine::nonnull<std::FILE> engine::system::fopen(engine::nonnull<std::filesystem::path::value_type const> path, engine::nonnull<char const> mode)
{
    if (std::FILE *fp = std::fopen(path, mode); fp == nullptr) {
        int const error = errno;
        throw std::system_error(error, std::system_category(), fmt::format("fopen({:?}, {:?}) failed", path, mode));
    } else {
        return fp;
    }
}
