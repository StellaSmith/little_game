#include <engine/Config.hpp>
#include <engine/Game.hpp>
#include <utils/error.hpp>

#include <SDL.h>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>
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

static void terminate_handler()
{
    auto const stacktrace = boost::stacktrace::stacktrace();
    fmt::print(stderr, "{}", boost::stacktrace::to_string(stacktrace));
}

int main(int argc, char **argv)
{
    std::set_terminate(&terminate_handler);

#ifndef NDEBUG
    if (auto logger = spdlog::default_logger(); logger->level() < spdlog::level::debug)
        logger->set_level(spdlog::level::debug);
#endif

#ifdef SDL_MAIN_HANDLED
    SDL_SetMainReady();
#endif
    for (int i = 0; i < argc; ++i) {
        if (argv[i] == "-h"sv || argv[i] == "--help"sv) {
            fmt::print(
                "Usage:\n"
                "\t{} [options]\n"
                "Options:\n"
                "\t-h\t--help\t\tDisplay this message and exit.\n"
                "\t-v\t--verbose\tDisplay more verbose messages.\n"
                "\t--sdl-video-drivers\tEnumerate the available video drivers.\n"
                "\t--sdl-audio-drivers\tEnumerate the available audio drivers.\n",
                argv[0]);
            return 0;
        } else if (argv[i] == "--sdl-video-drivers"sv) {
            int const n = SDL_GetNumVideoDrivers();
            spdlog::info("Available video drivers ({}):", n);
            for (int i = 0; i < n; ++i)
                spdlog::info("\t- {}\n", SDL_GetVideoDriver(i));
        } else if (argv[i] == "--sdl-audio-drivers"sv) {
            int const n = SDL_GetNumAudioDrivers();
            spdlog::info("Available audio drivers ({}):", n);
            for (int i = 0; i < n; ++i)
                spdlog::info("\t- {}\n", SDL_GetAudioDriver(i));
        } else if (argv[i] == "-v"sv || argv[i] == "--verbose"sv) {
            spdlog::info("Verbose output enabled");
        }
    }

    auto &config = engine::Config::load("./config/engine.json");

    if (int const error = SDL_Init(0); error != 0) {
        spdlog::trace("Failed to initialize SDL ({}): {}", error, SDL_GetError());
        throw std::runtime_error(SDL_GetError());
    }

    {
        char const *video_driver = config.sdl.video_driver ? config.sdl.video_driver->c_str() : nullptr;
        if (int const error = SDL_VideoInit(video_driver); error != 0) {
            spdlog::trace("Failed to initialize SDL Video subsystem ({}): {}", error, SDL_GetError());
            throw std::runtime_error(SDL_GetError());
        }
    }

    {
        char const *audio_driver = config.sdl.audio_driver ? config.sdl.audio_driver->c_str() : nullptr;
        if (int const error = SDL_AudioInit(audio_driver); error != 0) {
            spdlog::trace("Failed to initialize SDL Audio subsystem ({}): {}", error, SDL_GetError());
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

    if (!config.imgui.font_path) {
        if (!imgui_io.Fonts->AddFontFromFileTTF(config.imgui.font_path->c_str(), 14)) {
            spdlog::warn("[ImGui] failed to load font {}, using default font", *config.imgui.font_path);
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
