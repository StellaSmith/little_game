#ifndef UTILS_FILE_HANDLE_HPP
#define UTILS_FILE_HANDLE_HPP

#include <engine/result.hpp>
#include <engine/system/open_file.hpp>

#include <boost/interprocess/mapped_region.hpp>

#include <cstdio>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace engine {

    struct FileDeleter {
        void operator()(std::FILE *fp) const noexcept
        {
            std::fclose(fp);
        }
    };

    template <typename Ptr>
    struct FileBytes;

    template <typename Ptr>
    struct FileLines;

    template <typename Ptr>
    struct FileLock;

    struct File : public std::unique_ptr<std::FILE, FileDeleter> {
        using super = std::unique_ptr<std::FILE, FileDeleter>;
        using super::super;

        [[nodiscard]]
        std::shared_ptr<std::FILE> shared() && noexcept
        {
            return std::shared_ptr<std::FILE>(std::move(*this));
        }

        [[nodiscard]]
        static File open(std::filesystem::path const &path, char const *mode)
        {
            auto fp = engine::system::fopen(path, mode);
            return File(fp);
        }

        [[nodiscard]]
        FileBytes<std::FILE *> bytes() & noexcept;

        [[nodiscard]]
        FileBytes<File> bytes() && noexcept;

        [[nodiscard]]
        FileLines<std::FILE *> lines() & noexcept;

        [[nodiscard]]
        FileLines<File> lines() && noexcept;

        [[nodiscard]]
        FileLock<std::FILE *> lock() & noexcept;

        [[nodiscard]]
        FileLock<File> lock() && noexcept;
    };

    namespace impl {
        struct FileBytesBase {
            explicit FileBytesBase(std::FILE *fp) noexcept
                : m_fp(fp)
            {
            }

            [[nodiscard]]
            std::span<std::byte const> span();
            [[nodiscard]]
            std::vector<std::byte> vector() &;
            [[nodiscard]]
            std::vector<std::byte> vector() &&;
            [[nodiscard]]
            std::string string() &;
            [[nodiscard]]
            std::string string() &&;

        private:
            std::FILE *m_fp;
            std::variant<std::monostate, boost::interprocess::mapped_region, std::vector<std::byte>, std::string> m_cache;
        };
    }

    // for fancy pointers (e.g: std::unique_ptr, std::shared_ptr)
    template <typename Ptr>
    struct FileBytes : public impl::FileBytesBase {
        explicit FileBytes(Ptr fp) noexcept
            : impl::FileBytesBase(std::to_address(fp))
            , m_fp(std::move(fp))
        {
        }

    private:
        Ptr m_fp;
    };

    template <>
    struct FileBytes<std::FILE *> : public impl::FileBytesBase {
        explicit FileBytes(std::FILE *fp) noexcept
            : impl::FileBytesBase(fp)
        {
        }
    };
}

inline engine::FileBytes<std::FILE *> engine::File::bytes() & noexcept { return FileBytes(this->get()); }
inline engine::FileBytes<engine::File> engine::File::bytes() && noexcept { return FileBytes(std::move(*this)); }

#endif
