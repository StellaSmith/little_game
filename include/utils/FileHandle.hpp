#ifndef UTILS_FILE_HANDLE_HPP
#define UTILS_FILE_HANDLE_HPP

#include <cstdio>
#include <memory>

namespace utils {

    struct FileDeleter {
        void operator()(std::FILE *fp) const noexcept
        {
            std::fclose(fp);
        }
    };

    using FileHandle = std::unique_ptr<std::FILE, FileDeleter>;

}

#endif