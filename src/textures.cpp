#include "engine/textures.hpp"

#include "utils/error.hpp"

#include <glad/gl.h>
#include <nlohmann/json.hpp>
#include <stb_image.h>

#include <cstdio>
#include <filesystem>
#include <fmt/format.h>
#include <memory_resource>
#include <string>
#include <string_view>
#include <unordered_map>

using json = nlohmann::json;
using namespace std::literals;

static FILE *open_path(std::filesystem::path const &p, char const *mode)
{
    return std::fopen(p.string().c_str(), mode);
}

engine::Textures engine::load_textures()
{
    std::FILE *fp = std::fopen("assets/texture_pack.json", "r");
    if (!fp)
        utils::show_error("Can't open file assets/texture_pack.json");
    json texture_pack;
    try {
        texture_pack = json::parse(fp, nullptr, true, true);
    } catch (std::exception &e) {
        std::fclose(fp);
        utils::show_error("Error parsing file assets/texture_pack.json\n"s + e.what());
    }

    std::fclose(fp);

    int max_layers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers);

    using map_type = std::pmr::unordered_map<std::string_view, std::string_view>;

    auto buffer = std::make_unique<char[]>(std::min(max_layers, 1024) * sizeof(typename map_type::value_type));
    std::pmr::monotonic_buffer_resource buffer_resource;
    map_type textures(&buffer_resource);

    try {
        texture_pack.at("textures").get_to(textures);
    } catch (std::exception &e) {
        utils::show_error("Error obtaining textures\n"s + e.what());
    }

    if (textures.size() > static_cast<unsigned>(max_layers)) {
        std::string str_error = fmt::format("The maximun amount of textures in your system is {} but {}  where specified.", max_layers, textures.size());
        utils::show_error(str_error);
    }

    Textures result;

    int width = -1, height = -1;

    int i = 0;
    for (auto [name, name_path] : textures) {
        // ignore all until a ':'
        if (auto idx = name_path.find(':'); idx != std::string_view::npos)
            name_path = name_path.substr(idx + 1);

        auto const path = std::filesystem::current_path() / "assets"sv / name_path;

        int x, y, c;
        FILE *f = open_path(path, "rb");
        if (!f) {
            std::string str_error;
            if constexpr (std::is_same_v<typename std::filesystem::path::value_type, char>)
                str_error = fmt::format("Error openning image {}", path.native());
            else
                str_error = fmt::format("Error openning image {}", path.string());
            utils::show_error(str_error);
        }
        stbi_uc *buf = stbi_load_from_file(f, &x, &y, &c, 4);
        std::fclose(f);
        if (!buf) {
            std::string str_error;
            if constexpr (std::is_same_v<typename std::filesystem::path::value_type, char>)
                str_error = fmt::format("Error loading image  {}\n{}", path.native(), stbi_failure_reason());
            else
                str_error = fmt::format("Error loading image  {}\n{}", path.string(), stbi_failure_reason());
            utils::show_error(str_error);
        }

        if (width == -1) { // first iteration
            width = x;
            height = y;

            glGenTextures(1, &result.texture2d_array);
            glBindTexture(GL_TEXTURE_2D_ARRAY, result.texture2d_array);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, width, height, textures.size(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        } else if (x != width || y != height) {
            utils::show_error("Image sizes differ!");
        }
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf);
        stbi_image_free(buf);
        result.name_to_index.emplace(name, i);
        ++i;
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    return result;
}

#include "engine/game.hpp"

int engine::Game::get_texture_index(std::string_view name) const noexcept
{
    auto it = std::find_if(m_textures.name_to_index.begin(), m_textures.name_to_index.end(), [name](auto const &p) {
        return p.first == name;
    });
    if (it == m_textures.name_to_index.end())
        return -1;
    else
        return it->second;
}