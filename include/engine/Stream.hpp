#ifndef ENGINE_STREAM_HPP
#define ENGINE_STREAM_HPP

#include <boost/outcome/success_failure.hpp>
#include <resources_generated.hpp> // auto-generated

#include <engine/Result.hpp>
#include <utils/FileHandle.hpp>

#include <fmt/std.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <span>
#include <system_error>

#ifdef _WIN32
#include <cassert>
#include <cstring> // std::strlen
#include <stdio.h> // _wfopen_s
#else
#include <errno.h>
#endif

namespace engine {
    inline static engine::Result<utils::FileHandle> open_file(std::filesystem::path const &path, char const *mode) noexcept
    {
#if _WIN32
        wchar_t buf[32] {};
        std::size_t const size = std::strlen(mode);
        if (size >= std::size(buf))
            return boost::outcome_v2::failure(std::errc::argument_list_too_long);
        std::copy(mode, mode + size, buf);
        std::FILE *fp = nullptr;
        int const error = ::_wfopen_s(&fp, path.native().c_str(), &buf[0]);
#else
        std::FILE *fp = std::fopen(path.native().c_str(), mode);
        int const error = errno;
#endif
        if (!fp) {
            spdlog::error("failed to open {}", path);
            return boost::outcome_v2::failure(static_cast<std::errc>(error));
        }

        if constexpr (BUFSIZ < 1024 * 8)
            std::setvbuf(fp, nullptr, _IOFBF, 1024 * 8);

        return utils::FileHandle { fp };
    }

    engine::Result<resources::BaseResource const *> open_resource(std::string_view path) noexcept;
    engine::Result<std::span<std::byte const>> load_resource(std::string_view path) noexcept;
}
#endif
