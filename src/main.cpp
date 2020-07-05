#include "glDebug.h"

#include <glad/glad.h>
#include <SDL.h>
#include <glm/glm.hpp>

#include <cstdlib>
#include <cstdio>
#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <fstream>

using namespace std::literals;

std::string load_file(std::istream &is)
{
    return {std::istreambuf_iterator<char>{is}, std::istreambuf_iterator<char>{}};
}

std::string load_file(std::string_view path)
{
    std::ifstream stream;
    stream.open(path.data());
    if (!stream.is_open())
        throw std::runtime_error("Can't open file");
    return load_file(stream);
}

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

    // ready to begin drawing and processing user input

    glClearColor(0.0, 0.25, 0.5, 1.0);

    struct Vertex
    {
        float x, y, z;
        std::uint8_t r, g, b, a;
    }; // total of 32 bytes per vertex

    Vertex vertices[] = {
        {+0.0, -0.5, +0.0, 255u, 0u, 0u, 255u},
        {-0.5, +0.5, +0.0, 0u, 255u, 0u, 255u},
        {+0.5, +0.5, +0.0, 0u, 0u, 255u, 255u}};

    GLuint shader_program;

    {
        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

        std::string const vertex_shader_source_str = load_file("assets/basic.vert"sv);
        std::string const fragment_shader_source_str = load_file("assets/basic.frag"sv);

        char const *const vertex_shader_source = vertex_shader_source_str.c_str();
        char const *const fragment_shader_source = fragment_shader_source_str.c_str();

        glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
        glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);

        glCompileShader(vertex_shader);
        glCompileShader(fragment_shader);

        int success;
        char infoLog[512]{};
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertex_shader, 512, nullptr, infoLog);
            SDL_Log("OpenGL: Error compiling vertex shader: %s", infoLog);
            std::exit(EXIT_FAILURE);
        }
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertex_shader, 512, nullptr, infoLog);
            SDL_Log("OpenGL: Error compiling fragment shader: %s", infoLog);
            std::exit(EXIT_FAILURE);
        }

        shader_program = glCreateProgram();

        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, fragment_shader);
        glLinkProgram(shader_program);

        glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader_program, 512, nullptr, infoLog);
            SDL_Log("OpenGL: Error linking shader: %s", infoLog);
            std::exit(EXIT_FAILURE);
        }

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }

    GLuint vao; // vertex array object
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo; // vertex buffer object
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void *)12);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    bool running = true;
    SDL_Event event;
    while (running)
    {
        while (SDL_PollEvent(&event) && running)
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        // not currently using depth nor stencil but anyways
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glUseProgram(shader_program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // swap the buffer (present to the window surface)
        SDL_GL_SwapWindow(window);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
