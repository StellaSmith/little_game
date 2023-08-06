#include <engine/system/block_size.hpp>

#include <stdio.h> // fileno
#include <sys/stat.h> // fstat, struct stat (linux: statx, struct statx)

#ifdef __linux__
#include <fcntl.h> // AT_*
#endif

#ifdef __linux__
static bool has_statx() noexcept
{
    struct ::statx statxbuf;
    return ::statx(AT_FDCWD, "", AT_EMPTY_PATH, 0, &statxbuf) != -1;
}

static engine::result<std::size_t, std::errc> block_size_statx(int const fd) noexcept
{
    struct ::statx statxbuf;
    // stx_blksize is always provided, no need to add or check for a mask
    if (::statx(fd, "", AT_EMPTY_PATH, 0, &statxbuf) == -1) {
        return static_cast<std::errc>(errno);
    } else {
        return static_cast<std::size_t>(statxbuf.stx_blksize);
    }
}
#endif

static engine::result<std::size_t, std::errc> block_size_stat(int const fd) noexcept
{
    struct ::stat64 statbuf;
    if (::fstat64(fd, &statbuf) == -1) {
        return static_cast<std::errc>(errno);
    } else {
        return statbuf.st_blksize;
    }
}

engine::result<std::size_t, std::errc> engine::system::block_size(engine::nonnull<std::FILE> fp) noexcept
{

#ifdef __linux__
    static auto const block_size_impl = has_statx() ? &block_size_statx : &block_size_stat;
#else
    static auto const block_size_impl = &block_size_stat;
#endif

    int const fd = ::fileno(fp);
    if (fd == -1) return static_cast<std::errc>(errno);
    return block_size_impl(fd);
}