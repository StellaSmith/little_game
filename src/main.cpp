#include <SDL.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#include <cstdio>
#include <cstdlib>
#include <string>

#include "engine/chunk_t.hpp"
#include "engine/game.hpp"
#include "glDebug.h"

[[noreturn]] static void show_error(std::string_view msg, SDL_Window *w = nullptr)
{
    if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR!", msg.data(), w) < 0)
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", msg.data());
    std::exit(EXIT_FAILURE);
}

json g_config_engine;

bool g_verbose = false;

using namespace std::literals;

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

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
        } else if (argv[i] == "-v"sv || argv[i] == "--verbose"sv) {
            g_verbose = true;
        }
    }

    {
        lua_State *L = luaL_newstate();
        luaL_openlibs(L);
        lua_pushboolean(L, g_verbose);
        lua_setglobal(L, "_VERBOSE");
        luaL_dostring(L, R"(print("Hello from Lua!") if _VERBOSE then print("_VERBOSE is set to true") end)");
        lua_close(L);
    }

    if (SDL_Init(0) < 0)
        show_error("Error initializing SDL: "s + SDL_GetError());

    {
        char const *fname = "./cfg/engine.json";
        FILE *fp = std::fopen(fname, "r");
        if (!fp)
            show_error("Error opening engine configuration file. ("s + fname + ")");

        try {
            g_config_engine = json::parse(fp, nullptr, true, true);
        } catch (std::exception &e) {
            std::string error_str = "Error parsing engine config file.\n"s + e.what();
            std::fclose(fp);
            if (SDL_ShowSimpleMessageBox(SDL_MessageBoxFlags::SDL_MESSAGEBOX_ERROR, "EXCEPTION!", error_str.c_str(), nullptr) < 0)
                SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "%s", error_str.c_str());

            throw;
        }
        std::fclose(fp);
    }
    {
        std::string video_driver;
        try {
            g_config_engine.at("/SDL/video_driver"_json_pointer).get_to(video_driver);
        } catch (json::type_error const &e) {
            show_error("SDL Video Driver must be a string\n"s + e.what());
        } catch (json::out_of_range const &) {
            // do nothing
        }
        if (SDL_VideoInit(video_driver.empty() ? nullptr : video_driver.data()) < 0)
            show_error("Error initializing SDL Video subsystem: "s + SDL_GetError());
    }
    {
        std::string audio_driver;
        try {
            g_config_engine.at("/SDL/audio_driver"_json_pointer).get_to(audio_driver);
        } catch (json::type_error const &e) {
            show_error("SDL Audio Driver must be a string.\n"s + e.what());
        } catch (json::out_of_range const &) {
            // do nothing
        }
        if (SDL_AudioInit(audio_driver.empty() ? nullptr : audio_driver.data()) < 0)
            show_error("Error initializing SDL Audio subsystem: "s + SDL_GetError());
    }
    {
        auto get_integer = [](json::json_pointer const &path, unsigned &i) {
            try {
                json::const_reference node = g_config_engine.at(path);
                node.get_to(i);
            } catch (json::out_of_range const &e) {
                return; // use default if value is not present
            } catch (json::type_error const &e) {
                show_error(path.to_string() + " isnt an unsigned integer.\n" + e.what());
            }
        };

        unsigned red_bits = 3, green_bits = 3, blue_bits = 2, alpha_bits = 0, depth_bits = 24, stencil_bits = 0;
        get_integer("/SDL/OpenGL/red_bits"_json_pointer, red_bits);
        get_integer("/SDL/OpenGL/green_bits"_json_pointer, green_bits);
        get_integer("/SDL/OpenGL/blue_bits"_json_pointer, blue_bits);
        get_integer("/SDL/OpenGL/alpha_bits"_json_pointer, alpha_bits);
        get_integer("/SDL/OpenGL/depth_bits"_json_pointer, depth_bits);
        get_integer("/SDL/OpenGL/stencil_bits"_json_pointer, stencil_bits);

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        if (SDL_GL_SetAttribute(SDL_GL_RED_SIZE, red_bits) < 0)
            show_error("Error setting SDL_GL_RED_SIZE to " + std::to_string(red_bits));
        if (SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, green_bits) < 0)
            show_error("Error setting SDL_GL_GREEN_SIZE to " + std::to_string(green_bits));
        if (SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, blue_bits) < 0)
            show_error("Error setting SDL_GL_BLUE_SIZE to " + std::to_string(blue_bits));
        if (SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, alpha_bits) < 0)
            show_error("Error setting SDL_GL_ALPHA_SIZE to " + std::to_string(alpha_bits));
        if (SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depth_bits) < 0)
            show_error("Error setting SDL_GL_DEPTH_SIZE to " + std::to_string(depth_bits));
        if (SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencil_bits) < 0)
            show_error("Error setting SDL_GL_STENCIL_SIZE to" + std::to_string(stencil_bits));
    }

#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    SDL_Window *window = SDL_CreateWindow(
        /*Title*/ "My little game",
        /*Position (x, y)*/ SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        /*Size (width, height)*/ width, height,
        /*Flags*/ SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!window)
        show_error("Can't create main window: "s + SDL_GetError());

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    if (!gl_context)
        show_error("Can't create OpenGL 3.3 context: "s + SDL_GetError(), window);

    if (SDL_GL_MakeCurrent(window, gl_context) < 0)
        show_error("Can't set OpenGL context current: "s + SDL_GetError(), window);

    if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
        show_error("GLAD Error: Failed to initialize the OpenGL context.", window);

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

        std::printf("OpenGL %d.%d\n\tRGBA bits: %d, %d, %d, %d\n\tDepth bits: %d\n\tStencil bits: %d\n", major, minor, r, g, b, a, d, s);
        std::printf("\tVersion: %s\n\tVendor: %s\n\tRenderer: %s\n\tShading language version: %s\n", version, vendor, renderer, shading_version);
    }
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

    if (!IMGUI_CHECKVERSION())
        show_error("ImGui version mismatch!\nYou may need to recompile the game.", window);

    ImGui::CreateContext();
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
        try {
            auto const font_path = g_config_engine.at("/ImGui/font_path"_json_pointer).get<std::string>();

            if (!imgui_io.Fonts->AddFontFromFileTTF(font_path.data(), 14)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "IMGUI: Error loading font \"%.*s\", using default font.", static_cast<int>(font_path.size()), font_path.data());
                if (!imgui_io.Fonts->AddFontDefault())
                    show_error("IMGUI: Can't load default font."sv, window);
            }
        } catch (json::out_of_range const &e) {
            if (!imgui_io.Fonts->AddFontDefault())
                show_error("IMGUI: Can't load default font!"sv, window);
        } catch (json::type_error const &e) {
            show_error("ImGui font_path must be a string.\n"s + e.what());
        }
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
