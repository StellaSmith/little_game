#include <SDL.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "Config.hpp"
#include "engine/chunk_t.hpp"
#include "engine/game.hpp"
#include "glDebug.h"

static SDL_NORETURN void show_error(std::string_view msg, SDL_Window *w = nullptr)
{
    if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR!", msg.data(), w) < 0)
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", msg.data());
    std::exit(EXIT_FAILURE);
}

Config g_config_engine;

bool g_verbose = false;

using namespace std::literals;

int main(int argc, char **argv)
{
    constexpr int width = 640, height = 480;

    for (int i = 0; i < argc; ++i) {
        if (argv[i] == "--sdl-video-drivers"sv) {
            int drivers = SDL_GetNumVideoDrivers();
            std::printf("Available video drivers (%d):\n", drivers);
            for (int i = 0; i < drivers; ++i)
                std::printf("\t%d) %s\n", i + 1, SDL_GetVideoDriver(i));
        } else if (argv[i] == "--sdl-audio-drivers"sv) {
            int drivers = SDL_GetNumAudioDrivers();
            std::printf("Available audio drivers (%d):\n", drivers);
            for (int i = 0; i < drivers; ++i)
                std::printf("\t%d) %s\n", i + 1, SDL_GetAudioDriver(i));
        } else if (argv[i] == "-v" || argv[i] == "--verbose") {
            g_verbose = true;
        }
    }

    if (SDL_Init(0) < 0)
        show_error("Error initializing SDL: "s + SDL_GetError());

    {
        char const *fname = "./cfg/engine.cfg";
        FILE *fp = std::fopen(fname, "r");
        if (!fp) show_error("Error opening engine configuration file. ("s + fname + ")");

        try {
            g_config_engine = Config::from_file(fp);
        } catch (std::exception &e) {
            std::fclose(fp);
            if (SDL_ShowSimpleMessageBox(SDL_MessageBoxFlags::SDL_MESSAGEBOX_ERROR, "EXCEPTION!", e.what(), nullptr) < 0)
                SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "%s", e.what());

            throw;
        }
        std::fclose(fp);
    }
    {
        auto video_driver = g_config_engine.get_or("sdl_video_driver"sv, {});
        if (SDL_VideoInit(video_driver.empty() ? nullptr : video_driver.data()) < 0)
            show_error("Error initializing SDL Video subsystem: "s + SDL_GetError());
    }
    {
        auto audio_driver = g_config_engine.get_or("sdl_audio_driver"sv, {});
        if (SDL_AudioInit(audio_driver.empty() ? nullptr : audio_driver.data()) < 0)
            show_error("Error initializing SDL Audio subsystem: "s + SDL_GetError());
    }
    {
        auto get_integer = [](std::string_view v, unsigned &i) {
            auto o_integer = g_config_engine.get(v);
            if (o_integer.has_value()) {
                auto [ptr, ec] = std::from_chars(o_integer->data(), o_integer->data() + o_integer->size(), i);
                if (ec != std::errc {}) {
                    std::string error_str;
                    error_str += v;
                    error_str += " is not an unsigned integer"sv;
                    show_error(error_str);
                }
            }
        };

        unsigned red_bits = 3, green_bits = 3, blue_bits = 2, alpha_bits = 0, depth_bits = 24, stencil_bits = 0;
        get_integer("sdl_gl_red_size"sv, red_bits);
        get_integer("sdl_gl_green_size"sv, green_bits);
        get_integer("sdl_gl_blue_size"sv, blue_bits);
        get_integer("sdl_gl_alpha_size"sv, alpha_bits);
        get_integer("sdl_gl_depth_size"sv, depth_bits);
        get_integer("sdl_gl_stencil_size"sv, stencil_bits);

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        if (SDL_GL_SetAttribute(SDL_GL_RED_SIZE, red_bits) < 0)
            show_error("Error setting SDL_GL_RED_SIZE");
        if (SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, green_bits) < 0)
            show_error("Error setting SDL_GL_GREEN_SIZE");
        if (SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, blue_bits) < 0)
            show_error("Error setting SDL_GL_BLUE_SIZE");
        if (SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, alpha_bits) < 0)
            show_error("Error setting SDL_GL_ALPHA_SIZE");
        if (SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depth_bits) < 0)
            show_error("Error setting SDL_GL_DEPTH_SIZE");
        if (SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencil_bits) < 0)
            show_error("Error setting SDL_GL_STENCIL_SIZE");
    }

#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    SDL_Window *window = SDL_CreateWindow(
        /*Title*/ "My little game",
        /*Position (x, y)*/ SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        /*Size (width, height)*/ width, height,
        /*Flags*/ SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!window) show_error("Can't create main window: "s + SDL_GetError());

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    if (!gl_context) show_error("Can't create OpenGL 3.3 context: "s + SDL_GetError(), window);

    if (SDL_GL_MakeCurrent(window, gl_context) < 0)
        show_error("Can't set OpenGL context current: "s + SDL_GetError(), window);

    if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
        show_error("GLAD Error: Failed to initialize the OpenGL context.", window);

    {
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

        std::printf("OpenGL %d.%d\n\tRGBA bits: %d, %d, %d, %d\n\tDepth bits: %d\n\tStencil bits: %d\n", major, minor, r, g, b, a, d, s);
        std::printf("\tVersion: %s\n\tVendor: %s\n\tRenderer: %s\n\tShading language version: %s\n", version, vendor, renderer, shading_version);

        if (SDL_GL_ExtensionSupported("GL_KHR_debug")) {
            auto pfn_glDebugMessageCallback = reinterpret_cast<PFN_glDebugMessageCallback>(SDL_GL_GetProcAddress("glDebugMessageCallback"));
            auto pfn_glDebugMessageControl = reinterpret_cast<PFN_glDebugMessageControl>(SDL_GL_GetProcAddress("glDebugMessageControl"));

            int flags;
            glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
            if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                pfn_glDebugMessageCallback(glDebugOutput, nullptr);
                pfn_glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
                std::puts("\tDebug output enabled.\n");
            }
        }
    }

    if (!IMGUI_CHECKVERSION()) show_error("ImGui version mismatch!\nYou may need to recompile the game.", window);

    ImGuiContext *imgui_context = ImGui::CreateContext();
    ImGuiIO &imgui_io = ImGui::GetIO();
    imgui_io.IniFilename = "cfg/imgui.ini";
    imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER))
        imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    // TODO: Make color scheme of imgui configurable
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context); // always returns true

    // See imgui/examples/imgui_impl_opengl3.cpp
    ImGui_ImplOpenGL3_Init("#version 330 core"); // always returns true

    {
        auto o_imgui_font_path = g_config_engine.get("imgui_font_path"sv);
        if (o_imgui_font_path.has_value()) {
            if (!imgui_io.Fonts->AddFontFromFileTTF(o_imgui_font_path->data(), 14)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "IMGUI: Error loading font \"%.*s\", using default font.", o_imgui_font_path->size(), o_imgui_font_path->data());
                if (!imgui_io.Fonts->AddFontDefault())
                    show_error("IMGUI: Can't load default font."sv, window);
            }
        } else if (!imgui_io.Fonts->AddFontDefault())
            show_error("IMGUI: Can't load default font."sv, window);
    }

    engine::Game game;
    game.start();

    auto start = engine::Game::clock_type::now();

    bool show_demo_window = true;

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
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        game.update(delta);
        if (!game.running) break;
        game.render();

        // Draw ImGui on top of the game stuff
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // swap the buffer (present to the window surface)
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_AudioQuit(); // ERROR: Crashes when exiting pulseaudio, it tries to lock a (SDL) mutex, which are disabled.
    SDL_VideoQuit();
    SDL_Quit();

    return 0;
}
