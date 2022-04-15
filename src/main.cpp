#include <engine/Config.hpp>
#include <engine/Game.hpp>
#include <glDebug.h>
#include <utils/error.hpp>

#include <SDL.h>
#include <fmt/format.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <system_error>

#ifdef __linux__
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <SDL.hpp>

using namespace std::literals;

bool g_verbose = false;
static void fix_current_directory(char const *argv0);

int main(int argc, char **argv)
{
#ifndef NDEBUG
    if (auto logger = spdlog::default_logger(); logger->level() < spdlog::level::debug)
        logger->set_level(spdlog::level::debug);
#endif

#ifdef SDL_MAIN_HANDLED
    SDL_SetMainReady();
#endif
    try {
        for (int i = 0; i < argc; ++i) {
            if (argv[i] == "-h"sv || argv[i] == "--help"sv) {
                fmt::print(
                    "Usage:\n"
                    "\t{} [options]\n"
                    "Options:\n"
                    "\t-h\t--help\t\tDisplay this message and exit.\n"
                    "\t-v\t--verbose\tDisplay more verbose messages.\n"
                    "\t--sdl-video-drivers\tEnumerate the aviable video drivers.\n"
                    "\t--sdl-audio-drivers\tEnumerate the avaible audio drivers.\n",
                    argv[0]);
                return 0;
            } else if (argv[i] == "--sdl-video-drivers"sv) {
                int drivers = SDL_GetNumVideoDrivers();
                spdlog::info("Available video drivers ({}):\n", drivers);
                for (int i = 0; i < drivers; ++i)
                    spdlog::info("\t{}) {}\n", i + 1, SDL_GetVideoDriver(i));
            } else if (argv[i] == "--sdl-audio-drivers"sv) {
                auto const drivers = SDL::Audio::drivers();
                spdlog::info("Available audio drivers ({}):\n", drivers.size());
                for (char const *driver : drivers)
                    spdlog::info("\t{}) {}\n", i + 1, driver);

            } else if (argv[i] == "-v"sv || argv[i] == "--verbose"sv) {
                g_verbose = true;
                spdlog::info("Verbose output enabled");
            }
        }
        fix_current_directory(argv[0]);

        auto sdl = SDL { 0 };
        auto &config = engine::load_engine_config();
        sdl.init_video(config.sdl.video_driver.c_str());
        sdl.init_audio(config.sdl.audio_driver.c_str());

        constexpr int width = 640, height = 480;

        auto window = sdl
                          .video()
                          .window_builder()
                          .set_title("My little game")
                          .set_dimensions(width, height)
                          .resizable()
                          .shown()
                          .opengl()
                          .set_gl_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3)
                          .set_gl_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 3)
                          .set_gl_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE)
                          .set_gl_attribute(SDL_GL_RED_SIZE, config.opengl.red_bits)
                          .set_gl_attribute(SDL_GL_GREEN_SIZE, config.opengl.green_bits)
                          .set_gl_attribute(SDL_GL_BLUE_SIZE, config.opengl.blue_bits)
                          .set_gl_attribute(SDL_GL_ALPHA_SIZE, config.opengl.alpha_bits)
                          .set_gl_attribute(SDL_GL_DEPTH_SIZE, config.opengl.depth_bits)
                          .set_gl_attribute(SDL_GL_STENCIL_SIZE, config.opengl.stencil_bits)
#ifndef NDEBUG
                          .set_gl_attribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG)
#endif
                          .create();

#ifndef NDEBUG
        if (SDL_SetRelativeMouseMode(SDL_TRUE))
            utils::show_error("Can't set mouse to relative mode!"sv);
#endif

        auto gl_context = window.opengl_context();
        gl_context.make_current();

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&SDL_GL_GetProcAddress)))
            utils::show_error("GLAD Error."sv, "Failed to initialize the OpenGL context."sv);

        if (g_verbose) {
            int major, minor, r, g, b, a, d, s;
            SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
            SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
            SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &r);
            SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
            SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &b);
            SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);
            SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &d);
            SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &s);

            char unsigned const *version = glGetString(GL_VERSION);
            char unsigned const *vendor = glGetString(GL_VENDOR);
            char unsigned const *renderer = glGetString(GL_RENDERER);
            char unsigned const *shading_version = glGetString(GL_SHADING_LANGUAGE_VERSION);

            spdlog::info("OpenGL profile: {}.{}", major, minor);
            spdlog::info("\tRGBA bits: {}, {}, {}, {}", r, g, b, a);
            spdlog::info("\tDepth bits: {}", d);
            spdlog::info("\tStencil bits: {}", s);
            spdlog::info("\tVersion: {}", version);
            spdlog::info("\tVendor: {}", vendor);
            spdlog::info("\tRendered: {}", renderer);
            spdlog::info("\tShading language version: {}", shading_version);
        }
#if defined(GL_KHR_debug)
        if (GLAD_GL_KHR_debug) {
            int flags;
            glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
            if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback(glDebugOutput, nullptr);
                glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
                if (g_verbose)
                    std::puts("\tDebug output enabled.\n");
            }
        }
#endif
        SDL_GL_SetSwapInterval(0);
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

        ImGui_ImplSDL2_InitForOpenGL(window.get(), gl_context.get()); // always returns true

        // See imgui/examples/imgui_impl_opengl3.cpp
        ImGui_ImplOpenGL3_Init("#version 330 core"); // always returns true

        if (!config.imgui.font_path.empty()) {
            if (!imgui_io.Fonts->AddFontFromFileTTF(config.imgui.font_path.data(), 14)) {
                spdlog::error("[ImGui] Can't load font \"{}\", using default font.", config.imgui.font_path);
                if (!imgui_io.Fonts->AddFontDefault())
                    utils::show_error("ImGui Error."sv, "Can't load default font."sv);
            }
        } else {
            if (!imgui_io.Fonts->AddFontDefault())
                utils::show_error("ImGui Error."sv, "Can't load default font!"sv);
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

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(window.get());
            ImGui::NewFrame();

            game.update(delta);
            if (!game.running) break;
            game.render();

            // Draw ImGui on top of the game stuff
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // swap the buffer (present to the window surface)
            SDL_GL_SwapWindow(window.get());
        }

        game.cleanup();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        return 0;
    } catch (sol::error const &e) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Lua panic!", e.what(), nullptr);
        spdlog::critical("Lua panic!: {}", e.what());
        throw;
    } catch (utils::application_error const &e) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, e.title().data(), e.body().data(), nullptr);
        spdlog::critical("{}: {}", e.title(), e.body());
        throw;
    } catch (std::exception const &e) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "EXCEPTION NOT HANDLED!!", e.what(), nullptr);
        spdlog::critical("std::exception raised: {}", e.what());
        throw;
    } catch (...) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "UNKOWN EXCEPTION NOT HANDLED!!!", "UNKOWN EXCEPTION NOT HANDLED!!!", nullptr);
        spdlog::critical("Unkown exception raised");
        throw;
    }
    return -1;
}

static void fix_current_directory(char const *argv0)
{
#ifdef __unix__
    {
        auto path = std::string { argv0 };
        if (path.back() == '/') path.pop_back();

        // go up two levels, one for bin/

        if (auto pos = path.rfind('/'); pos != std::string::npos) {
            path.resize(pos);
        }

        if (auto pos = path.rfind('/'); pos != std::string::npos) {
            path.resize(pos);
        }

        if (chdir(path.c_str()) < 0) {
            int const err = errno;
            std::string const err_str = std::system_category().message(err);
            utils::show_error("Couldn't chdir", fmt::format("Couldn't change current working directory to {}\n{}", path, err_str));
        }
        if (g_verbose)
            spdlog::info("Working directory: {}", path);
    }
#elif defined(_WIN32)
    {
        std::vector<wchar_t> buf(MAX_PATH);
        DWORD err;
        while (true) {
            DWORD len = GetModuleFileNameW(nullptr, buf.data(), buf.size());
            if ((err = GetLastError()) == ERROR_INSUFFICIENT_BUFFER || len == buf.size()) {
                buf.resize(buf.size() * 2);
            } else {
                break;
            }
        }

        if (err)
            throw std::system_error(err, std::system_category(),
                "GetModuleFileNameW failed: Couldn't get executable path");

        *wcsrchr(buf.data(), L'\\') = L'\0';
        if (auto *p = wcsrchr(buf.data(), L'\\'); p) {
            *p = L'\0';
        } else {
            utils::show_error("Couldn't chdir", "Couldn't change current working above root");
        }

        auto len = std::wcslen(buf.data());
        if (SetCurrentDirectoryW(buf.data()) == 0) {
            err = GetLastError();
            auto narrow_buf = std::make_unique<char[]>(len * 4 + 1);
            WideCharToMultiByte(CP_ACP, 0, buf.data(), -1, narrow_buf.get(), len * 4 + 1, nullptr, nullptr);
            throw std::system_error(err, std::system_category(),
                fmt::format("SetCurrentDirectoryW failed: Couldn't change current working directory to {}", narrow_buf.get()));
        }
    }
#else
#error Unsupported OS.\
Please add a way to change the current directory to the executable installation path here for your OS\
and do a pull request.
#endif
}