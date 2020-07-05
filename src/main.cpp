#include "glDebug.h"
#include "engine/chunk_renderer.hpp"
#include "engine/chunk_t.hpp"

#include <glad/glad.h>
#include <SDL.h>
#include <glm/glm.hpp>
#include <stb_image.h>

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
            SDL_LogCritical(SDL_LOG_CATEGORY_RENDER, "OpenGL: Error linking shader:\n%s", infoLog);
            std::exit(EXIT_FAILURE);
        }

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }

    GLuint vao; // vertex array object
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    engine::chunk_t chunk;

    chunk.blocks[0].id = 1;

    engine::chunk_renderer::chunk_meshes chunk_meshes{0, 0, 0, 0};

    glGenBuffers(1, &chunk_meshes.solid_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, chunk_meshes.solid_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 20, (void *)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 20, (void *)12);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &chunk_meshes.translucent_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, chunk_meshes.translucent_buffer);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 20, (void *)0);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 20, (void *)12);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    int x, y, c;
    unsigned char *data = stbi_load("assets/Phosphophyllite.jpg", &x, &y, &c, 4);
    std::fprintf(stderr, "Channels: %d\n", c);
    if (!data)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "STB: Can't load image: %s", stbi_failure_reason());
        std::exit(EXIT_FAILURE);
    }
    GLuint texture0;
    glGenTextures(1, &texture0);
    glBindTexture(GL_TEXTURE_2D, texture0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(shader_program);
    glUniform1i(glGetUniformLocation(shader_program, "texture0"), 0);

    engine::chunk_renderer::generate_mesh(chunk_meshes, chunk);

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

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture0);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, chunk_meshes.solid_buffer);
        glDrawArrays(GL_TRIANGLES, 0, chunk_meshes.solid_vertices);

        glBindBuffer(GL_ARRAY_BUFFER, chunk_meshes.translucent_buffer);
        glDrawArrays(GL_TRIANGLES, 0, chunk_meshes.translucent_vertices);

        // swap the buffer (present to the window surface)
        SDL_GL_SwapWindow(window);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glDeleteBuffers(1, &chunk_meshes.solid_buffer);
    glDeleteBuffers(1, &chunk_meshes.translucent_buffer);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
