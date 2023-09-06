#include <engine/Config.hpp>
#include <engine/File.hpp>
#include <engine/resources.hpp>
#include <utils/error.hpp>

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
        SPDLOG_CRITICAL("tried to obtain uninitialized engine config");
        std::terminate();
    }

    return s_config;
}

engine::Config const &engine::Config::load(std::filesystem::path const &path)
{
    if (s_config_loaded.exchange(true) == true)
        THROW_CRITICAL("{}", "engine config already loaded");

    auto content = [&]() {
        if (auto maybe_fp = engine::File::open(path, "r"); maybe_fp.has_error()) {
            THROW_CRITICAL("error opening engine configuration file {}: {}", path, std::make_error_code(maybe_fp.error()).message());
        } else {
            return maybe_fp.value().bytes().string();
        }
    }();

    rapidjson::Document doc;
    {
        using namespace rapidjson;
        doc.ParseInsitu<kParseCommentsFlag | kParseTrailingCommasFlag | kParseNanAndInfFlag>(content.data());
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

    auto get_string = [&](char const *json_pointer) -> std::optional<std::string> {
        if (auto *pointer = rapidjson::Pointer(json_pointer).Get(doc); pointer && pointer->IsString()) {
            return std::string { pointer->GetString(), pointer->GetStringLength() };
        } else if (pointer) {
            utils::show_error("Error loading engine config file."sv, fmt::format("{} must be an string.", json_pointer));
        }
        return std::nullopt;
    };

    auto get_integer = [&doc](char const *json_pointer) -> std::optional<int> {
        if (auto *pointer = rapidjson::Pointer(json_pointer).Get(doc); pointer && pointer->IsInt()) {
            return pointer->GetInt();
        } else if (pointer) {
            utils::show_error("Error loading engine config file."sv, fmt::format("{} must be an integer", json_pointer));
        }
        return std::nullopt;
    };

    s_config.sdl.video_driver = get_string("/SDL/video_driver");
    s_config.sdl.audio_driver = get_string("/SDL/audio_driver");
    s_config.imgui.font_path = get_string("/ImGui/font_path");

    if (auto maybe_root = get_string("/folders/root"))
        s_config.folders.root = std::move(*maybe_root);
    if (auto maybe_cache = get_string("/folders/cache"))
        s_config.folders.cache = std::move(*maybe_cache);

    s_config.opengl.red_bits = get_integer("/SDL/OpenGL/red_bits");
    s_config.opengl.green_bits = get_integer("/SDL/OpenGL/green_bits");
    s_config.opengl.blue_bits = get_integer("/SDL/OpenGL/blue_bits");
    s_config.opengl.alpha_bits = get_integer("/SDL/OpenGL/alpha_bits");
    s_config.opengl.depth_bits = get_integer("/SDL/OpenGL/depth_bits");
    s_config.opengl.stencil_bits = get_integer("/SDL/OpenGL/stencil_bits");

    return s_config;
}
