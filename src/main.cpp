#include <version>

#include <engine/Config.hpp>
#include <engine/Game.hpp>
#include <utils/error.hpp>

#include <SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fmt/std.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>

using namespace std::literals;

[[gnu::constructor]]
static void set_terminate_handler()
{
    static std::terminate_handler const default_handler = std::get_terminate();

    auto handler = []() {
        auto const stacktrace = boost::stacktrace::stacktrace();
        for (auto const &frame : stacktrace)
            spdlog::critical("{}", fmt::streamed(frame)); // do not use the macro, location printed by it can be confused

        default_handler();
    };
    std::set_terminate(+handler);
}

static std::vector<char const *> audio_drivers()
{
    int const n = SDL_GetNumAudioDrivers();
    std::vector<char const *> result;
    result.reserve(n);

    for (int i = 0; i < n; ++i)
        result.push_back(SDL_GetAudioDriver(i));
    return result;
}

static std::vector<char const *> video_drivers()
{
    int const n = SDL_GetNumVideoDrivers();
    std::vector<char const *> result;
    result.reserve(n);

    for (int i = 0; i < n; ++i)
        result.push_back(SDL_GetVideoDriver(i));
    return result;
}

int main(int argc, char **argv)
{
#ifdef SDL_MAIN_HANDLED
    SDL_SetMainReady();
#endif
    std::filesystem::path config_path = "config/engine.json";

    for (int i = 0; i < argc; ++i) {
        if (argv[i] == "-h"sv || argv[i] == "--help"sv) {
            fmt::print(
                "Usage:\n"
                "\t{program} [options]\n"
                "Options:\n"
                "\t-h --help\t\tDisplay this message and exit.\n"
                "\t-v --verbose\tDisplay more verbose messages.\n"
                "\t   --sdl-video-drivers\tEnumerate the available video drivers.\n"
                "\t   --sdl-audio-drivers\tEnumerate the available audio drivers.\n"
                "\t-c --config <path>\tPath to engine configuration file (default: {default_config_path})\n",
                fmt::arg("program", argv[0]),
                fmt::arg("default_config_path", config_path));
            return 0;
        } else if (argv[i] == "--sdl-video-drivers"sv) {
            SPDLOG_INFO("available video drivers: {}", video_drivers());
        } else if (argv[i] == "--sdl-audio-drivers"sv) {
            SPDLOG_INFO("available audio drivers: {}", audio_drivers());
        } else if (argv[i] == "-v"sv || argv[i] == "--verbose"sv) {
            SPDLOG_INFO("verbose output enabled");
        } else if (argv[i] == "-c"sv || argv[i] == "--config"sv) {
            char const *used = argv[i];
            if (char const *raw_config_path = argv[++i]; raw_config_path == nullptr) {
                SPDLOG_ERROR("missing path argument for {}", used);
                throw std::invalid_argument(fmt::format("missing path argument for {}", used));
            } else {
                config_path = raw_config_path;
            }
        }
    }

    auto &config = engine::Config::load(config_path);

    if (int const error = SDL_Init(0); error != 0) {
        SPDLOG_ERROR("Failed to initialize SDL ({}): {}", error, SDL_GetError());
        throw std::runtime_error(SDL_GetError());
    }

    {
        char const *video_driver = config.sdl.video_driver ? config.sdl.video_driver->c_str() : nullptr;
        if (int const error = SDL_VideoInit(video_driver); error != 0) {
            SPDLOG_ERROR("Failed to initialize SDL Video subsystem ({}): {}", error, SDL_GetError());
            throw std::runtime_error(SDL_GetError());
        }
    }

    {
        char const *audio_driver = config.sdl.audio_driver ? config.sdl.audio_driver->c_str() : nullptr;
        if (int const error = SDL_AudioInit(audio_driver); error != 0) {
            SPDLOG_ERROR("Failed to initialize SDL Audio subsystem ({}): {}", error, SDL_GetError());
            throw std::runtime_error(SDL_GetError());
        }
    }

    if (!IMGUI_CHECKVERSION())
        utils::show_error("ImGui version mismatch!\nYou may need to recompile the game."sv);

    ImGui::CreateContext();

    ImGuiIO &imgui_io = ImGui::GetIO();
    imgui_io.IniFilename = "cfg/imgui.ini";
    imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER))
        imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    // TODO: Make color scheme of imgui configurable
    ImGui::StyleColorsDark();

    if (config.imgui.font_path) {
        if (!imgui_io.Fonts->AddFontFromFileTTF(config.imgui.font_path->c_str(), 14)) {
            SPDLOG_WARN("[ImGui] failed to load font {}, using default font", *config.imgui.font_path);
            if (!imgui_io.Fonts->AddFontDefault())
                utils::show_error("ImGui Error."sv, "failed to load default font"sv);
        }
    } else {
        if (!imgui_io.Fonts->AddFontDefault())
            utils::show_error("ImGui Error."sv, "failed load default font"sv);
    }

    engine::Game game;
    game.start();

    auto start = engine::Game::clock_type::now();

    SDL_Event event;
    while (game.running) {
        auto now = engine::Game::clock_type::now();
        auto delta = now - start;
        start = now;

        while (SDL_PollEvent(&event) && game.running) {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            ImGui_ImplSDL2_ProcessEvent(&event);
            game.input(event);
        }

        game.update(delta);
        if (!game.running) break;
        game.render();
    }

    game.cleanup();

    return 0;
}
