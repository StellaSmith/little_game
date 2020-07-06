#include "glDebug.h"
#include "engine/chunk_renderer.hpp"
#include "engine/chunk_t.hpp"
#include "engine/game.hpp"

#include <glad/glad.h>
#include <SDL.h>

#include <cstdlib>
#include <cstdio>
#include <string>

int main(int argc, char **argv)
{
    constexpr int width = 640, height = 480;
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_Log("SDL Error: %s", SDL_GetError());
        std::exit(EXIT_FAILURE);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    SDL_Window *window = SDL_CreateWindow(
        /*Title*/ "My little game",
        /*Position (x, y)*/ SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        /*Size (width, height)*/ width, height,
        /*Flags*/ SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (window == nullptr)
    {
        SDL_Log("SDL Error: %s", SDL_GetError());
        std::exit(EXIT_FAILURE);
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    if (gl_context == nullptr)
    {
        SDL_Log("SDL Error: %s", SDL_GetError());
        std::exit(EXIT_FAILURE);
    }

    if (SDL_GL_MakeCurrent(window, gl_context) < 0)
    {
        SDL_Log("SDL Error: %s", SDL_GetError());
        std::exit(EXIT_FAILURE);
    }

    if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
    {
        SDL_Log("GLAD Error: Failed to initialize the OpenGL context.");
        std::exit(EXIT_FAILURE);
    }

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

        GLint max_vertex_attribs;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs);
        std::printf("\tMax vertex attribs: %d\n", max_vertex_attribs);

        if (SDL_GL_ExtensionSupported("GL_KHR_debug"))
        {
            auto pfn_glDebugMessageCallback = reinterpret_cast<PFN_glDebugMessageCallback>(SDL_GL_GetProcAddress("glDebugMessageCallback"));
            auto pfn_glDebugMessageControl = reinterpret_cast<PFN_glDebugMessageControl>(SDL_GL_GetProcAddress("glDebugMessageControl"));

            int flags;
            glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
            if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
            {
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                pfn_glDebugMessageCallback(glDebugOutput, nullptr);
                pfn_glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
                std::puts("\tDebug output enabled.\n");
            }
        }
    }

    engine::Game game;
    game.start();

    auto start = engine::Game::clock_type::now();

    SDL_Event event;
    while (game.running)
    {
        auto now = engine::Game::clock_type::now();
        auto delta = now - start;
        start = now;

        while (SDL_PollEvent(&event) && game.running)
            game.input(event);

        game.update(delta);
        if (!game.running)
            break;
        game.render();

        // swap the buffer (present to the window surface)
        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
