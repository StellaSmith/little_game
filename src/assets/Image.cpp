#include <engine/File.hpp>
#include <engine/assets/Image.hpp>
#include <engine/sdl/RWOps.hpp>

#include <SDL_image.h>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <fmt/std.h>
#include <spdlog/spdlog.h>

using namespace std::literals;

class SDLImageError final : public std::runtime_error {
private:
    using std::runtime_error::runtime_error;

public:
    [[nodiscard]]
    static SDLImageError current()
    {
        return SDLImageError(IMG_GetError());
    }
};

engine::assets::Image::Image(std::string_view name)
    : IAsset(name)
{
}
#include <engine/sdl/RWOps.hpp>
void engine::assets::Image::load(std::span<std::byte const> bytes, std::optional<std::string_view> format)
{
    SPDLOG_INFO("loading image from memory");
    auto rw_ops = engine::sdl::RWOps::from_const_memory(bytes);

    // since the format must be null-terminated, it gets converted to a std::string
    SDL_Surface *surface = IMG_LoadTyped_RW(rw_ops.raw(), false, format ? std::string(*format).c_str() : nullptr);
    if (!surface) {
        auto error = SDLImageError::current();
        SPDLOG_ERROR("failed to load image: {}", error.what());
        throw std::move(error);
    }

    m_surface.reset(surface);
}

void engine::assets::Image::load(std::filesystem::path const &path)
{
    {
        SPDLOG_INFO("trying to memory map file {:?}", path);
        boost::interprocess::file_mapping file_mapping;
        boost::interprocess::mapped_region mapped_region;
        try {
            file_mapping = boost::interprocess::file_mapping(path.native().c_str(), boost::interprocess::read_only);
            mapped_region = boost::interprocess::mapped_region(file_mapping, boost::interprocess::read_only);
        } catch (boost::interprocess::interprocess_exception const &e) {
            SPDLOG_ERROR("failed to memory map file {:?}: {}", path, e.what());
            goto read_ops;
        }
        auto const ptr = static_cast<std::byte const *>(mapped_region.get_address());
        auto const len = mapped_region.get_size();

        auto const extension = path.extension().string();
        auto const format = [&]() -> std::optional<std::string_view> {
            if (extension == ""sv || extension == "."sv) // either the file didn't have an extension or the filename ends with a dot
                return std::nullopt;
            else
                return std::string_view { extension }.substr(1);
        }();

        return Image::load(std::span<std::byte const> { ptr, len }, format);
    }
read_ops:

    SPDLOG_INFO("loading image from file {:?}", path);
    auto fp = [&]() {
        auto maybe_fp = engine::File::open(path, "r");
        if (!maybe_fp.has_value()) {
            auto error_code = std::make_error_code(maybe_fp.error());
            SPDLOG_ERROR("failed to to open file {:?}: {}", path, error_code.message());
            throw std::system_error(error_code);
        }
        return std::move(maybe_fp).value();
    }();

    auto rw_ops = engine::sdl::RWOps::from_fp(std::move(fp));
    SDL_Surface *surface = IMG_Load_RW(rw_ops.raw(), SDL_FALSE);
    if (!surface) {
        SPDLOG_ERROR("failed to load image: {}", IMG_GetError());
        throw std::runtime_error(IMG_GetError());
    }

    m_surface.reset(surface);
}

void engine::assets::Image::save(std::filesystem::path const &path, std::string_view format) const
{
    SPDLOG_INFO("saving image to file {}", path);
    if (SDL_strncasecmp(format.data(), "bmp", format.size()) == 0) {
        if (SDL_SaveBMP(m_surface.get(), path.c_str()) != 0) {
            SPDLOG_ERROR("failed to save image: {}", SDL_GetError());
            throw std::runtime_error(SDL_GetError());
        }
    } else if (SDL_strncasecmp(format.data(), "png", format.size()) == 0) {
        if (IMG_SavePNG(m_surface.get(), path.c_str()) != 0) {
            SPDLOG_ERROR("failed to save image: {}", IMG_GetError());
            throw std::runtime_error(IMG_GetError());
        }
    } else if (SDL_strncasecmp(format.data(), "jpg", format.size()) == 0) {
        if (IMG_SaveJPG(m_surface.get(), path.c_str(), 100) != 0) {
            SPDLOG_ERROR("failed to save image: {}", IMG_GetError());
            throw std::runtime_error(IMG_GetError());
        }
    } else {
        SPDLOG_ERROR("unsupported image format: {:?}", format);
        throw std::invalid_argument(fmt::format("unsupported image format: {:?}", format));
    }
}

void engine::assets::Image::save(std::span<std::byte> bytes, std::string_view format) const
{
    SPDLOG_INFO("saving image to memory");

    auto *rwops = SDL_RWFromMem(bytes.data(), bytes.size());
    if (SDL_strncasecmp(format.data(), "bmp", format.size()) == 0) {
        if (SDL_SaveBMP_RW(m_surface.get(), rwops, 1) != 0) {
            SPDLOG_ERROR("failed to save image: {}", SDL_GetError());
            throw std::runtime_error(SDL_GetError());
        }
    } else if (SDL_strncasecmp(format.data(), "png", format.size()) == 0) {
        if (IMG_SavePNG_RW(m_surface.get(), rwops, 1) != 0) {
            SPDLOG_ERROR("failed to save image: {}", IMG_GetError());
            throw std::runtime_error(IMG_GetError());
        }
    } else if (SDL_strncasecmp(format.data(), "jpg", format.size()) == 0) {
        if (IMG_SaveJPG_RW(m_surface.get(), rwops, 1, 100) != 0) {
            SPDLOG_ERROR("failed to save image: {}", IMG_GetError());
            throw std::runtime_error(IMG_GetError());
        }
    } else {
        SPDLOG_ERROR("unsupported image format: {:?}", format);
        throw std::invalid_argument(fmt::format("unsupported image format: {:?}", format));
    }
}