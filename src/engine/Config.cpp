

#include <engine/Config.hpp>
#include <rapidjson/error/error.h>
#include <rapidjson/reader.h>
#include <string_view>
#include <utils/error.hpp>

#include <fmt/format.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/pointer.h>

#include <cstdio>
#include <cstring>
#include <memory>

static engine::Config s_config {};

using namespace std::literals;

engine::Config const &engine::config()
{
    return s_config;
}

engine::Config const &engine::load_engine_config()
{

    char const *fname = "./cfg/engine.json";
    FILE *fp = std::fopen(fname, "r");
    if (!fp)
        utils::show_error(fmt::format("Error opening engine configuration file. ({})", fname));

    std::fseek(fp, 0, SEEK_END);
    std::size_t filesize = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);
    auto content = std::make_unique<char[]>(filesize + 1);
    if (!std::fread(content.get(), 1, filesize, fp)) {
        if (std::ferror(fp))
            utils::show_error(fmt::format("Error reading engine configuration file. ({})", fname));
    }
    std::fclose(fp);

    rapidjson::Document doc;
    {
        using namespace rapidjson;
        doc.ParseInsitu<kParseInsituFlag | kParseCommentsFlag | kParseTrailingCommasFlag | kParseNanAndInfFlag>(content.get());
    }

    if (doc.HasParseError()) {
        auto error = doc.GetParseError();
        auto offset = doc.GetErrorOffset();
        char const *message = rapidjson::GetParseError_En(error);

        std::size_t lineno = 0;
        std::size_t lineat = 0;

        for (std::size_t i = 0; i < offset; ++i) {
            if (content[i] == '\n') {
                lineno = 0;
                lineat = i;
            }
        }

        utils::show_error("Error parsing engine config file."sv,
            fmt::format("Error at line {} column {}: {}"sv, lineno + 1, offset - lineat, message));
    }
    s_config = engine::Config {};

    if (auto *value = rapidjson::Pointer("/SDL/video_driver").Get(doc); value) {
        s_config.sdl.video_driver = std::string { value->GetString(), value->GetStringLength() };
    } else if (value) {
        utils::show_error("Error loading engine config file."sv, "/SDL/video_driver must be an string");
    }

    if (auto *value = rapidjson::Pointer("/SDL/audio_driver").Get(doc); value && value->IsString()) {
        s_config.sdl.audio_driver = std::string { value->GetString(), value->GetStringLength() };
    } else if (value) {
        utils::show_error("Error loading engine config file."sv, "/SDL/audio_driver must be an string");
    }

    auto set_integer = [&doc](char const *path, int &value) {
        if (auto *pointer = rapidjson::Pointer(path).Get(doc); pointer && pointer->IsInt()) {
            value = pointer->GetInt();
        } else if (pointer) {
            utils::show_error("Error loading engine config file."sv, fmt::format("{} must be an integer", path));
        }
    };

    set_integer("/SDL/OpenGL/red_bits", s_config.opengl.red_bits);
    set_integer("/SDL/OpenGL/green_bits", s_config.opengl.green_bits);
    set_integer("/SDL/OpenGL/blue_bits", s_config.opengl.blue_bits);
    set_integer("/SDL/OpenGL/alpha_bits", s_config.opengl.alpha_bits);
    set_integer("/SDL/OpenGL/depth_bits", s_config.opengl.depth_bits);
    set_integer("/SDL/OpenGL/stencil_bits", s_config.opengl.stencil_bits);

    set_integer("/Terminal/max_lines", s_config.terminal.max_lines);

    if (auto *value = rapidjson::Pointer("/ImGui/font_path").Get(doc); value && value->IsString()) {
        s_config.imgui.font_path = std::string { value->GetString(), value->GetStringLength() };
    } else if (value) {
        utils::show_error("Error loading engine config file."sv, "/ImGui/font_path must be an string");
    }
    return s_config;
}
