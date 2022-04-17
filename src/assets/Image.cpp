#include <engine/assets/Image.hpp>
#include <engine/errors/UnsupportedFileType.hpp>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <spdlog/spdlog.h>
#include <stb_image.h>

engine::errors::UnsupportedImageFormat::UnsupportedImageFormat(engine::ImageFormat fmt) noexcept
    : m_format(fmt)
{
}

char const *engine::errors::UnsupportedImageFormat::what() const noexcept
{
    return "engine::errors::UnsupportedImageFormat";
}

static decltype(auto) string_path(std::filesystem::path const &p) noexcept
{
    if constexpr (std::is_same_v<std::filesystem::path::value_type, char>)
        return p.native();
    else
        return p.string();
}

using namespace std::literals;

engine::assets::Image engine::assets::Image::load(std::filesystem::path const &path, engine::ImageFormat desired_format)
{
    spdlog::info("Loading image from {}"sv, string_path(path));

    boost::interprocess::file_mapping file(path.string().c_str(), boost::interprocess::read_only);
    boost::interprocess::mapped_region mapped_region(file, boost::interprocess::read_only);

    auto const buffer = static_cast<stbi_uc const *>(mapped_region.get_address());
    int const len = mapped_region.get_size();
    int x, y;

    void *result = nullptr;
    if (desired_format.type() == ImageFormat::uint && desired_format.channel_width() == ImageFormat::b1)
        result = stbi_load_from_memory(buffer, len, &x, &y, nullptr, desired_format.channels());
    else if (desired_format.type() == ImageFormat::uint && desired_format.channel_width() == ImageFormat::b2)
        result = stbi_load_16_from_memory(buffer, len, &x, &y, nullptr, desired_format.channels());
    else if (desired_format.type() == ImageFormat::floating && desired_format.channel_width() == ImageFormat::b4)
        result = stbi_loadf_from_memory(buffer, len, &x, &y, nullptr, desired_format.channels());
    else
        throw engine::errors::UnsupportedImageFormat(desired_format);

    if (!result)
        throw engine::errors::UnsupportedFileType(stbi_failure_reason());

    auto image = Image { desired_format };
    image.m_width = x;
    image.m_height = y;
    image.m_data.reset(result);
    return image;
}