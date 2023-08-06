#include <engine/Config.hpp>
#include <engine/Stream.hpp>
#include <utils/error.hpp>
#include <utils/file.hpp>

#include <fmt/std.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/error/error.h>
#include <rapidjson/pointer.h>
#include <rapidjson/reader.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string_view>

static std::atomic_bool s_config_loaded;
static engine::Config s_config {};

using namespace std::literals;

engine::Config const &engine::config()
{
    if (!s_config_loaded.load()) {
        const auto message = "tried to obtain uninitialized engine config"sv;
        spdlog::critical("{}", message);
        throw std::runtime_error(message.data());
    }

    return s_config;
}

engine::Config const &engine::Config::load(std::filesystem::path const &path)
{
    if (s_config_loaded.exchange(true) == true)
        THROW_CRITICAL("{}", "engine config already loaded");

    auto content = [&]() {
        if (auto maybe_fp = engine::open_file(path, "r"); maybe_fp.has_error()) {
            THROW_CRITICAL("error opening engine configuration file {}: {}", path, std::make_error_code(maybe_fp.error()).message());
        } else {
            return utils::load_file(maybe_fp.value().get());
        }
    }();

    rapidjson::Document doc;
    {
        using namespace rapidjson;
        doc.ParseInsitu<kParseInsituFlag | kParseCommentsFlag | kParseTrailingCommasFlag | kParseNanAndInfFlag>(content.data());
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

    auto set_string = [&](char const *json_pointer, auto &value) {
        if (auto *pointer = rapidjson::Pointer(json_pointer).Get(doc); pointer && pointer->IsString()) {
            value = std::string { pointer->GetString(), pointer->GetStringLength() };
        } else if (pointer) {
            utils::show_error("Error loading engine config file."sv, fmt::format("{} must be an string.", json_pointer));
        }
    };

    auto set_integer = [&doc](char const *json_pointer, auto &value) {
        if (auto *pointer = rapidjson::Pointer(json_pointer).Get(doc); pointer && pointer->IsInt()) {
            value = pointer->GetInt();
        } else if (pointer) {
            utils::show_error("Error loading engine config file."sv, fmt::format("{} must be an integer", json_pointer));
        }
    };

    set_string("/SDL/video_driver", s_config.sdl.video_driver);
    set_string("/SDL/audio_driver", s_config.sdl.audio_driver);
    set_string("/ImGui/font_path", s_config.imgui.font_path);

    set_string("/folders/root", s_config.folders.root);
    set_string("/folders/cache", s_config.folders.cache);

    set_integer("/SDL/OpenGL/red_bits", s_config.opengl.red_bits);
    set_integer("/SDL/OpenGL/green_bits", s_config.opengl.green_bits);
    set_integer("/SDL/OpenGL/blue_bits", s_config.opengl.blue_bits);
    set_integer("/SDL/OpenGL/alpha_bits", s_config.opengl.alpha_bits);
    set_integer("/SDL/OpenGL/depth_bits", s_config.opengl.depth_bits);
    set_integer("/SDL/OpenGL/stencil_bits", s_config.opengl.stencil_bits);

    return s_config;
}
