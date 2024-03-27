#pragma once
#ifndef ENGINE_SDL_RWOPS_HPP
#define ENGINE_SDL_RWOPS_HPP

#include <engine/File.hpp>
#include <engine/sdl/Error.hpp>

#include <filesystem>
#include <memory>

#include <SDL_rwops.h>
#include <spdlog/spdlog.h>

namespace engine::sdl {

    class RWOps {
    public:
        explicit RWOps(SDL_RWops *rw_ops) noexcept
            : m_raw(rw_ops)
        {
        }

        [[nodiscard]]
        static RWOps from_const_memory(std::span<std::byte const> data)
        {
            if (data.size() > INT_MAX) {
                SPDLOG_ERROR("memory is too long");
                throw std::invalid_argument("memory is too long");
            }

            auto *rw_ops = SDL_RWFromConstMem(data.data(), data.size());
            if (!rw_ops) {
                SPDLOG_ERROR("failed to create SDL_RWops: {}", SDL_GetError());
                throw engine::sdl::Error::current();
            }
            return RWOps(rw_ops);
        }

        [[nodiscard]]
        static RWOps from_memory(std::span<std::byte> data)
        {
            if (data.size() > INT_MAX) {
                SPDLOG_ERROR("memory is too long");
                throw std::invalid_argument("memory is too long");
            }

            auto *rw_ops = SDL_RWFromMem(data.data(), data.size());
            if (!rw_ops) {
                SPDLOG_ERROR("failed to create SDL_RWops: {}", SDL_GetError());
                throw engine::sdl::Error::current();
            }
            return RWOps(rw_ops);
        }

        [[nodiscard]]
        static RWOps from_file(std::filesystem::path const &path, std::string const &mode)
        {
            return RWOps::from_file(path.u8string().c_str(), mode.c_str());
        }

        [[nodiscard]]
        static RWOps from_file(std::u8string const &path, std::string const &mode)
        {
            return RWOps::from_file(path.c_str(), mode.c_str());
        }

        [[nodiscard]]
        static RWOps from_file(char8_t const *path, char const *mode)
        {
            auto *rw_ops = SDL_RWFromFile((char const *)path, mode);
            if (!rw_ops) {
                SPDLOG_ERROR("failed to create SDL_RWops: {}", SDL_GetError());
                throw engine::sdl::Error::current();
            }
            return RWOps(rw_ops);
        }

        [[nodiscard]]
        static RWOps from_fp(std::FILE *file, bool close_on_destruct)
        {
            auto *rw_ops = SDL_RWFromFP(file, close_on_destruct ? SDL_TRUE : SDL_FALSE);
            if (!rw_ops) {
                SPDLOG_ERROR("failed to create SDL_RWops: {}", SDL_GetError());
                throw engine::sdl::Error::current();
            }
            return RWOps(rw_ops);
        }

        [[nodiscard]]
        static RWOps from_fp(engine::File file)
        {
            return RWOps::from_fp(file.release(), true);
        }

        void close() noexcept
        {
            m_raw.reset();
        }

        [[nodiscard]]
        std::size_t write(std::span<std::byte const> data)
        {
            auto written = SDL_RWwrite(m_raw.get(), data.data(), 1, data.size());
            if (written == 0) {
                SPDLOG_ERROR("failed to write to SDL_RWops: {}", SDL_GetError());
                throw engine::sdl::Error::current();
            }
            return written;
        }

        [[nodiscard]]
        std::size_t read(std::span<std::byte> data)
        {
            auto read = SDL_RWread(m_raw.get(), data.data(), 1, data.size());
            if (read == 0) {
                SPDLOG_ERROR("failed to read from SDL_RWops: {}", SDL_GetError());
                throw engine::sdl::Error::current();
            }
            return read;
        }

        enum class Whence {
            set = RW_SEEK_SET,
            current = RW_SEEK_CUR,
            end = RW_SEEK_END,
        };

        std::int64_t seek(std::int64_t offset, Whence whence)
        {
            auto position = SDL_RWseek(m_raw.get(), offset, static_cast<int>(whence));
            if (position < 0) {
                SPDLOG_ERROR("failed to seek in SDL_RWops: {}", SDL_GetError());
                throw engine::sdl::Error::current();
            }
            return position;
        }

        [[nodiscard]]
        std::int64_t tell() const
        {
            return const_cast<RWOps *>(this)->seek(0, Whence::current);
        }

        [[nodiscard]]
        std::int64_t size() const
        {
            auto size = SDL_RWsize(m_raw.get());
            if (size < 0) {
                SPDLOG_ERROR("failed to get size of SDL_RWops: {}", SDL_GetError());
                throw engine::sdl::Error::current();
            }
            return size;
        }

    private:
        struct Deleter {
            void operator()(SDL_RWops *rw_ops) const noexcept
            {
                if (SDL_RWclose(rw_ops) < 0)
                    SPDLOG_WARN("failed to close SDL_RWops: {}", SDL_GetError());
            }
        };

        std::unique_ptr<SDL_RWops, Deleter> m_raw;
    };

} // namespace

#endif
