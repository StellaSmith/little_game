#ifndef ENGINE_STREAM_HPP
#define ENGINE_STREAM_HPP

#include <cstdio>
#include <filesystem>
#include <resources.cpp>

#ifdef _WIN32
#include <stdio.h> // _wfopen
#endif

namespace engine {

    inline static std::FILE *open_file(std::filesystem::path const &p, char const *mode) noexcept
    {
#if _WIN32
        wchar_t buf[32] {};
        std::size_t sz = std::strlen(mode);
        assert(sz < std::size(buf));
        for (std::size_t i = 0; i < sz; ++i)
            buf[i] = mode[i];
        return ::_wfopen(p.native().c_str(), &buf[0]);
#else
        return std::fopen(p.native().c_str(), mode);
#endif
    }

    resources::BaseResource const *open_resource(std::string_view path) noexcept;

}
#endif
