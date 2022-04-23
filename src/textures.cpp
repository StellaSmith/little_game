#include <engine/Stream.hpp>
#include <engine/assets/Image.hpp>
#include <engine/textures.hpp>
#include <utils/error.hpp>

#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>
#include <fmt/format.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/pointer.h>
#include <stb_image.h>

#ifdef ENGINE_WITH_OPENGL
#include <glad/glad.h>
#endif
#ifdef ENGINE_WITH_VULKAN
#include <vulkan/vulkan.h>
#endif

#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>

using namespace std::literals;
namespace {
    struct ImageDeleter {
        void operator()(stbi_uc *buf) const noexcept
        {
            stbi_image_free(buf);
        }
    };
}

engine::Textures engine::load_textures()
{
    std::FILE *fp = std::fopen("assets/texture_pack.json", "r");
    if (!fp)
        utils::show_error("Can't open file assets/texture_pack.json");
    std::fseek(fp, 0, SEEK_END);
    std::size_t filesize = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);
    auto content = std::make_unique<char[]>(filesize + 1);
    std::fread(content.get(), 1, filesize, fp);
    std::fclose(fp);

    rapidjson::Document texture_pack;
    {
        using namespace rapidjson;
        texture_pack.ParseInsitu<kParseInsituFlag | kParseCommentsFlag | kParseTrailingCommasFlag | kParseNanAndInfFlag>(content.get());
    }
    if (texture_pack.HasParseError()) {
        auto error = texture_pack.GetParseError();
        auto offset = texture_pack.GetErrorOffset();
        char const *message = rapidjson::GetParseError_En(error);

        std::size_t lineno = 0;
        std::size_t lineat = 0;

        for (std::size_t i = 0; i < offset; ++i) {
            if (content[i] == '\n') {
                lineno = 0;
                lineat = i;
            }
        }

        utils::show_error("Error parsing file assets/texture_pack.json\n"sv,
            fmt::format("Error at line {} column {}: {}"sv, lineno + 1, offset - lineat, message));
    }

    using map_type = boost::container::small_flat_map<std::string_view, std::string_view, 16u>;
    map_type textures;

    if (auto *value = rapidjson::Pointer("/textures").Get(texture_pack); value && value->IsObject()) {
        for (auto const &[name_v, path_v] : value->GetObject()) {
            auto const name = std::string_view { name_v.GetString(), name_v.GetStringLength() };
            if (!path_v.IsString())
                utils::show_error("Error loading textures pack."sv, fmt::format("/textures/{} must be an string", name));
            auto const path = std::string_view { path_v.GetString(), path_v.GetStringLength() };
            textures.emplace(name, path);
        }
    } else if (value) {
        utils::show_error("Error loading textures pack."sv, "/textures must be an object");
    } else {
        utils::show_error("Error loading textures pack."sv, "Can't obtain /textures");
    }

#ifdef ENGINE_WITH_OPENGL
    int max_layers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers);
    if (textures.size() > static_cast<unsigned>(max_layers)) {
        std::string str_error = fmt::format("The maximun amount of textures in your system is {} but {}  where specified.", max_layers, textures.size());
        utils::show_error(str_error);
    }
#endif

    Textures result;

    int width = -1, height = -1;

    int i = 0;
    for (auto [name, name_path] : textures) {
        // ignore all until a ':'
        if (auto idx = name_path.find(':'); idx != std::string_view::npos)
            name_path = name_path.substr(idx + 1);

        auto const path = std::filesystem::current_path() / "assets"sv / name_path;
        auto const image = engine::assets::Image::load(path);

        if (width == -1) { // first iteration
            width = image.width();
            height = image.height();

#ifdef ENGINE_WITH_OPENGL
            glGenTextures(1, &result.texture2d_array);
            glBindTexture(GL_TEXTURE_2D_ARRAY, result.texture2d_array);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, width, height, textures.size(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
#endif
        } else if (image.width() != width || image.height() != height) {
            utils::show_error("Image sizes differ!");
        }
#ifdef ENGINE_WITH_OPENGL
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
#endif
        result.name_to_index.emplace(name, i);
        ++i;
    }
#ifdef ENGINE_WITH_OPENGL
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
#endif

    return result;
}

#include <engine/Game.hpp>

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