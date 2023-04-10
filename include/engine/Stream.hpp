#ifndef ENGINE_STREAM_HPP
#define ENGINE_STREAM_HPP

#include <resources_generated.hpp> // auto-generated
#include <utils/FileHandle.hpp>

#include <spdlog/spdlog.h>

#include <cstdio>
#include <filesystem>
#include <system_error>

#ifdef _WIN32
#include <cassert>
#include <cstring> // std::strlen
#include <stdio.h> // _wfopen_s
#else
#include <errno.h>
#endif

namespace engine {

    inline static utils::FileHandle open_file(std::filesystem::path const &path, char const *mode, std::error_code &ec) noexcept
    {
        std::FILE *fp;
        int err_nr;
#if _WIN32
        wchar_t buf[32] {};
        std::size_t sz = std::strlen(mode);
        assert(sz < sizeof(buf));
        for (std::size_t i = 0; i < sz; ++i)
            buf[i] = mode[i];
        err_nr = ::_wfopen_s(&fp, path.native().c_str(), &buf[0]);
#else
        fp = std::fopen(path.native().c_str(), mode);
        err_nr = errno;
#endif
        if (!fp) {
            ec.assign(err_nr, std::generic_category());
            spdlog::error("Failed to open {}", path.string());
        } else {
            ec.clear();
        }

        if constexpr (BUFSIZ < 1024 * 8)
            std::setvbuf(fp, nullptr, _IOFBF, 1024 * 8);

        return utils::FileHandle { fp };
    }

    inline static utils::FileHandle open_file(std::filesystem::path const &path, char const *mode)
    {
        std::error_code ec;
        auto fp = open_file(path, mode, ec);
        if (!fp)
            throw std::system_error(ec);
        return fp;
    }

    resources::BaseResource const *open_resource(std::string_view path) noexcept;

}
#endif
