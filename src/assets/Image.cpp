#include <engine/File.hpp>
#include <engine/assets/Image.hpp>
#include <engine/sdl/RWOps.hpp>

#include <SDL_image.h>
#include <SDL_surface.h>
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

void engine::assets::Image::load(std::span<std::byte const> bytes, std::optional<std::string_view> format)
{
    SPDLOG_INFO("loading image from memory");
    auto rw_ops = engine::sdl::RWOps::from_const_memory(bytes);

    // since the format must be null-terminated, it gets converted to a std::string
    m_surface = std::unique_ptr<SDL_Surface, Deleter>(IMG_LoadTyped_RW(rw_ops.get(), false, format ? std::string(*format).c_str() : nullptr));
    if (!m_surface) {
        auto error = SDLImageError::current();
        SPDLOG_ERROR("failed to load image: {}", error);
        throw std::move(error);
    }
}

void engine::assets::Image::load(std::filesystem::path const &path)
{
    {
        SPDLOG_INFO("trying to memory map file {:?}", path);
        boost::interprocess::file_mapping file_mapping;
        boost::interprocess::mapped_region mapped_region;
        try {
            file_mapping = boost::interprocess::file_mapping(path.c_str(), boost::interprocess::read_only);
            mapped_region = boost::interprocess::mapped_region(file_mapping, boost::interprocess::read_only);
        } catch (boost::interprocess::interprocess_exception const &e) {
            SPDLOG_ERROR("failed to memory map file {:?}: {}", path, e);
            goto read_ops;
        }

        auto const span = std::span<std::byte const>(
            static_cast<std::byte const *>(mapped_region.get_address()),
            mapped_region.get_size());

        auto const extension = path.extension().string();
        auto const format = [&]() -> std::optional<std::string_view> {
            if (extension == ""sv || extension == "."sv) // either the file didn't have an extension or the filename ends with a dot
                return std::nullopt;
            else
                return std::string_view(extension).substr(1);
        }();

        return Image::load(span, format);
    }
read_ops:

    SPDLOG_INFO("loading image from file {:?}", path);
    auto fp = engine::File::open(path, "r");

    auto rw_ops = engine::sdl::RWOps::from_fp(std::move(fp));
    m_surface = std::unique_ptr<SDL_Surface, Deleter>(IMG_Load_RW(rw_ops.get(), SDL_FALSE));
    if (!m_surface) {
        auto error = SDLImageError::current();
        SPDLOG_ERROR("failed to load image: {}", error.what());
        throw error;
    }
}

void engine::assets::Image::save(std::filesystem::path const &path, std::string_view format) const
{
    SPDLOG_INFO("saving image to file {}", path);
    if (SDL_strncasecmp(format.data(), "bmp", format.size()) == 0) {
        if (SDL_SaveBMP(m_surface.get(), path.c_str()) != 0) {
            auto error = SDLImageError::current();
            SPDLOG_ERROR("failed to save image: {}", error);
            throw std::move(error);
        }
    } else if (SDL_strncasecmp(format.data(), "png", format.size()) == 0) {
        if (IMG_SavePNG(m_surface.get(), path.c_str()) != 0) {
            auto error = SDLImageError::current();
            SPDLOG_ERROR("failed to save image: {}", error);
            throw std::move(error);
        }
    } else if (SDL_strncasecmp(format.data(), "jpg", format.size()) == 0) {
        if (IMG_SaveJPG(m_surface.get(), path.c_str(), 100) != 0) {
            auto error = SDLImageError::current();
            SPDLOG_ERROR("failed to save image: {}", error);
            throw std::move(error);
        }
    } else {
        SPDLOG_ERROR("unsupported image format: {:?}", format);
        throw std::invalid_argument(fmt::format("unsupported image format: {:?}", format));
    }
}

void engine::assets::Image::save(std::span<std::byte> bytes, std::string_view format) const
{
    SPDLOG_INFO("saving image to memory");

    auto rw_ops = engine::sdl::RWOps::from_memory(bytes);
    if (SDL_strncasecmp(format.data(), "bmp", format.size()) == 0) {
        if (SDL_SaveBMP_RW(m_surface.get(), rw_ops.get(), SDL_FALSE) != 0) {
            auto error = SDLImageError::current();
            SPDLOG_ERROR("failed to save image: {}", error);
            throw std::move(error);
        }
    } else if (SDL_strncasecmp(format.data(), "png", format.size()) == 0) {
        if (IMG_SavePNG_RW(m_surface.get(), rw_ops.get(), SDL_FALSE) != 0) {
            auto error = SDLImageError::current();
            SPDLOG_ERROR("failed to save image: {}", error);
            throw std::move(error);
        }
    } else if (SDL_strncasecmp(format.data(), "jpg", format.size()) == 0) {
        if (IMG_SaveJPG_RW(m_surface.get(), rw_ops.get(), SDL_FALSE, 100) != 0) {
            auto error = SDLImageError::current();
            SPDLOG_ERROR("failed to save image: {}", error);
            throw std::move(error);
        }
    } else {
        SPDLOG_ERROR("unsupported image format: {:?}", format);
        throw std::invalid_argument(fmt::format("unsupported image format: {:?}", format));
    }
}
