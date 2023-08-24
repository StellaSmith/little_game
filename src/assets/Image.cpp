#include <engine/File.hpp>
#include <engine/assets/Image.hpp>

#include <SDL_image.h>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <fmt/std.h>
#include <spdlog/spdlog.h>

template <typename T>
static std::string type_name(T const &obj)
{
    char const *name = typeid(obj).name();
#ifdef _WIN32
    return name
#else
    int status;
    auto real_name = std::unique_ptr<char[], utils::FreeDeleter>(abi::__cxa_demangle(name, nullptr, nullptr, &status));
    return real_name.get();
#endif
}

engine::assets::Image engine::assets::Image::load(std::span<std::byte const> bytes)
{
    SPDLOG_INFO("Loading image from memory");

    auto *rwops = SDL_RWFromConstMem(bytes.data(), bytes.size());
    SDL_Surface *surface = IMG_Load_RW(rwops, true);

    if (!surface) {
        SPDLOG_ERROR("Failed to load image: {}", IMG_GetError());
        throw std::runtime_error(IMG_GetError());
    }

    Image image;
    image.m_surface.reset(surface);
    return image;
}

engine::assets::Image engine::assets::Image::load(std::filesystem::path const &path)
{

    {
        SPDLOG_INFO("Memory mapping file {}", path);
        boost::interprocess::file_mapping file_mapping;
        boost::interprocess::mapped_region mapped_region;
        try {
            file_mapping = boost::interprocess::file_mapping(path.native().c_str(), boost::interprocess::read_only);
            mapped_region = boost::interprocess::mapped_region(file_mapping, boost::interprocess::read_only);
        } catch (boost::interprocess::interprocess_exception const &e) {
            SPDLOG_ERROR("Failed to memory map file {}: {}", path, e.what());
            goto read_ops;
        }
        auto const ptr = static_cast<std::byte const *>(mapped_region.get_address());
        auto const len = mapped_region.get_size();

        return Image::load(std::span<std::byte const> { ptr, len });
    }
read_ops:

    SPDLOG_INFO("Loading image from file {}", path);
    auto fp = [&]() {
        auto maybe_fp = engine::File::open(path, "r");
        if (!maybe_fp.has_value()) {
            auto error_code = std::make_error_code(maybe_fp.error());
            SPDLOG_ERROR("failed to to open file {}: {}", path, error_code.message());
            throw std::system_error(error_code);
        }
        return std::move(maybe_fp).value();
    }();

    auto *rwops = SDL_RWFromFP(fp.get(), SDL_FALSE);
    SDL_Surface *surface = IMG_Load_RW(rwops, SDL_TRUE);

    if (!surface) {
        SPDLOG_ERROR("Failed to load image: {}", IMG_GetError());
        throw std::runtime_error(IMG_GetError());
    }

    Image image;
    image.m_surface.reset(surface);
    return image;
}
