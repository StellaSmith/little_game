#include <engine/File.hpp>
#include <engine/system/block_size.hpp>
#include <engine/system/page_size.hpp>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <io.h>
#include <stdio.h>
#undef _fileno
#endif

#include <numeric>

namespace {

    template <class... Ts>
    struct overloaded : Ts... {
        using Ts::operator()...;
    };

    template <class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

}

template <typename T>
static T read_to(std::FILE *fp)
{
    T result;

    std::size_t block_size = 0;
    try {
        block_size = engine::system::block_size(fp);
    } catch (std::system_error const &) {
    }
    std::size_t const buffer_size = std::gcd(BUFSIZ, std::gcd(engine::system::page_size(), block_size));

    for (;;) {
        result.resize(result.size() + buffer_size);

        errno = 0;
        std::size_t const bytes_read = std::fread(result.data() + result.size() - buffer_size, 1, buffer_size, fp);
        if (bytes_read == 0) {
            if (std::ferror(fp))
                throw std::system_error(errno, std::generic_category(), "std::fread() failed");
            else
                break;
        }
        result.resize(result.size() - buffer_size + bytes_read);
    }
    return result;
}

static boost::interprocess::mapped_region map_to_memory(std::FILE *fp)
{
    struct HandleWrapper {
        std::FILE *fp;
        boost::interprocess::mapping_handle_t get_mapping_handle() const
        {
            boost::interprocess::mapping_handle_t result {};
#ifdef _WIN32
            int const fd = ::_fileno(fp);
            if (fd == -1 || fd == -2) throw std::system_error(errno, std::generic_category(), "_fileno() failed");
            intptr_t const handle = ::_get_osfhandle(fd);
            if (handle == -1 || handle == -2) throw std::system_error(errno, std::generic_category(), "_get_osfhandle() failed");
            result.handle = reinterpret_cast<void *>(handle);
#else
            int const fd = ::fileno(fp);
            if (fd == -1) throw std::system_error(errno, std::generic_category(), "fileno() failed");
            result.handle = fd;
#endif
            return result;
        }
    };

    return boost::interprocess::mapped_region(HandleWrapper(fp), boost::interprocess::read_only);
}

std::span<std::byte const> engine::impl::FileBytesBase::span()
{
    return std::visit(overloaded(
                          [&](std::monostate) mutable {
                              try {
                                  auto &region = m_cache.emplace<boost::interprocess::mapped_region>(map_to_memory(m_fp));
                                  return std::span<std::byte const>(reinterpret_cast<std::byte const *>(region.get_address()), region.get_size());
                              } catch (std::system_error const &ex) {
                                  SPDLOG_WARN("failed to memory map file: {}", ex.what());
                                  auto &v = m_cache.emplace<std::vector<std::byte>>(read_to<std::vector<std::byte>>(m_fp));
                                  return std::span<std::byte const>(v.data(), v.size());
                              }
                          },
                          [&](boost::interprocess::mapped_region &r) { return std::span<std::byte const>(reinterpret_cast<std::byte const *>(r.get_address()), r.get_size()); },
                          [&](std::vector<std::byte> &v) { return std::span<std::byte const>(v.data(), v.size()); },
                          [&](std::string &s) { return std::span<std::byte const>(reinterpret_cast<std::byte const *>(s.data()), s.size()); }),
        m_cache);
}

std::string engine::impl::FileBytesBase::string() &
{
    return std::visit(overloaded(
                          [&](std::monostate) mutable { return m_cache.emplace<std::string>(read_to<std::string>(m_fp)); },
                          [&](boost::interprocess::mapped_region &r) { return std::string(reinterpret_cast<char const *>(r.get_address()), r.get_size()); },
                          [&](std::vector<std::byte> &v) { return std::string(reinterpret_cast<char const *>(v.data()), v.size()); },
                          [&](std::string &s) { return std::string(std::move(s)); }),
        m_cache);
}

std::string engine::impl::FileBytesBase::string() &&
{
    return std::visit(overloaded(
                          [&](std::monostate) { return read_to<std::string>(m_fp); },
                          [&](boost::interprocess::mapped_region &r) { return std::string(reinterpret_cast<char const *>(r.get_address()), r.get_size()); },
                          [&](std::vector<std::byte> &v) { return std::string(reinterpret_cast<char const *>(v.data()), v.size()); },
                          [&](std::string &s) { return std::string(std::move(s)); }),
        m_cache);
}
