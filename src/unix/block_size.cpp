#include <engine/system/block_size.hpp>

#include <stdio.h> // fileno
#include <sys/stat.h> // fstat, struct stat (linux: statx, struct statx)
#include <system_error>

#ifdef __linux__
#include <fcntl.h> // AT_*
#endif

#ifdef __linux__
static bool has_statx() noexcept
{
    struct ::statx statxbuf;
    return ::statx(AT_FDCWD, "", AT_EMPTY_PATH, 0, &statxbuf) != -1;
}

static std::size_t block_size_statx(int const fd)
{
    struct ::statx statxbuf;
    // stx_blksize is always provided, no need to add or check for a mask
    if (::statx(fd, "", AT_EMPTY_PATH, 0, &statxbuf) == -1) {
        int const error = errno;
        throw std::system_error(error, std::system_category(), "statx() failed");
    } else {
        return statxbuf.stx_blksize;
    }
}
#endif

static std::size_t block_size_stat(int const fd)
{
    struct ::stat64 statbuf;
    if (::fstat64(fd, &statbuf) == -1) {
        int const error = errno;
        throw std::system_error(error, std::system_category(), "stat64() failed");
    } else {
        return statbuf.st_blksize;
    }
}

std::size_t engine::system::block_size(engine::nonnull<std::FILE> fp)
{
#ifdef __linux__
    static auto const block_size_impl = has_statx() ? &block_size_statx : &block_size_stat;
#else
    static auto const block_size_impl = &block_size_stat;
#endif

    int const fd = ::fileno(fp);
    if (fd == -1) {
        int const error = errno;
        throw std::system_error(error, std::system_category(), "fileno() failed");
    }
    return block_size_impl(fd);
}
