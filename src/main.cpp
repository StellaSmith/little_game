#include <glad/glad.h>
#include <SDL.h>
#include <cstdlib>
#include <cstdio>
#include <glm/glm.hpp>
#include <vector>
#include <array>

extern "C" void GLAPIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, const void *userParam);

struct Chunk {
    static constexpr std::size_t chunk_size = 16;
    
    enum Block {
        Air,
        Stone,
        Dirt,
        Grass
    };

    std::array<Block, chunk_size * chunk_size * chunk_size> m_blocks;

    GLuint m_solid_mesh;

    struct Vertex {
        glm::vec3 position;
        glm::vec4 color;
    };

    glm::vec4 color_for_block(Block const& block) {
        switch (block) {
        case Air:
            return { 0.0, 0.0, 0.0, 0.0 };
        case Stone:
            return { 0.5, 0.5, 0.5, 1.0 };
        case Dirt:
            return { 0.5, 50.0 / 255.0, 50.0 / 255.0, 1.0 };
        case Grass:
            return { 0.0, 100.0 / 255.0, 0.0, 1.0 };
        default:
            return { 242.0 / 255.0, 0.0, 242.0 / 255.0, 1.0 };
        }
    }

    auto const& get_block(std::size_t x, std::size_t y, std::size_t z) const
    {
        return m_blocks[ x * chunk_size * chunk_size + y * chunk_size + z];
    }

    auto& get_block(std::size_t x, std::size_t y, std::size_t z)
    {
        return m_blocks[ x * chunk_size * chunk_size + y * chunk_size + z ];
    }

    static constexpr bool is_solid(Block const& block) {
        return block != Air;
    }

    void generate_mesh() {
        std::vector<Vertex> vertices;

        for (std::size_t x = 0; x < chunk_size; ++x) {
            for (std::size_t y = 0; y < chunk_size; ++y) {
                for (std::size_t z = 0; z < chunk_size; ++z) {
                    auto const& block = get_block(x, y, z);

                    // -x
                    if (x != 0 && !is_solid(get_block(x - 1, y, z))) {
                        glm::vec4 const color = color_for_block(block);
                        vertices.push_back({ { x, y, z }, color });
                        vertices.push_back({ { x, y - 1, z }, color });
                        vertices.push_back({ { x, y - 1, z + 1}, color });

                        vertices.push_back({ { x, y, z }, color });
                        vertices.push_back({ { x, y, z + 1 }, color });
                        vertices.push_back({ { x, y - 1, z + 1 }, color });
                    }

                    // +x 
                    if (x != chunk_size - 1 && !is_solid(get_block(x + 1, y, z))) {
                        glm::vec4 const color = color_for_block(block);

                        vertices.push_back({ { x + 1, y, z}, color });
                        vertices.push_back({ { x + 1, y - 1, z}, color });
                        vertices.push_back({ { x + 1, y - 1, z + 1}, color });

                        vertices.push_back({ { x + 1, y, z}, color });
                        vertices.push_back({ { x + 1, y, z + 1}, color });
                        vertices.push_back({ { x + 1, y - 1, z + 1}, color });
                    }

                    // -y
                    if (y != 0 && !is_solid(get_block(x, y, z))) {
                        glm::vec4 const color = color_for_block(block);

                        vertices.push_back({ { x, y, z }, color });
                        vertices.push_back({ { x + 1, y, z}, color });
                        vertices.push_back({ { x + 1, y, z + 1 }, color });
                        
                        vertices.push_back({ { x, y, z }, color });
//                        vertices.push_back({  });
                    }
                }
            }
        }
    }
};

int main() {
    constexpr int width = 640, height = 480;
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
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
        /*Flags*/ SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    ); 

    if (window == nullptr) {
        SDL_Log("SDL Error: %s", SDL_GetError());
        std::exit(EXIT_FAILURE);
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    if (gl_context == nullptr) {
        SDL_Log("SDL Error: %s", SDL_GetError());
        std::exit(EXIT_FAILURE);
    }

    if (SDL_GL_MakeCurrent(window, gl_context) < 0) {
        SDL_Log("SDL Error: %s", SDL_GetError());
        std::exit(EXIT_FAILURE);
    }

    if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
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

        std::printf("OpenGL %d.%d\n\tRGBA bits: %d, %d, %d, %d\n\tDepth bits: %d\n\tStencil bits: %d\n", major, minor, r, g, b, a, d, s);

        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(glDebugOutput, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        }
    }

    // ready to begin drawing and processing user input

    glClearColor(0.0, 0.25, 0.5, 1.0);


    char const *vertex_shader_source = R"GLSL(
    #version 330 core
    
    layout (location = 0) in vec3 v_position;
    layout (location = 1) in vec4 v_color;

    out vec4 f_color;

    void main() {
        gl_Position = vec4(v_position, 1);
        f_color = v_color;
    }
    )GLSL";

    char const *fragment_shader_source = R"GLSL(
    #version 330 core

    in vec4 f_color;

    out vec4 color;

    void main() {
        color = f_color;
    }
    )GLSL";

    struct Vertex {
        float x, y, z;
        std::uint8_t r, g, b, a;
    }; // total of 32 bytes per vertex

    Vertex vertices[] = {
        {  0.0, -0.5, 0.0, 255u, 0u, 0u, 255u },
        { -0.5,  0.5, 0.0, 0u, 255u, 0u, 255u },
        {  0.5,  0.5, 0.0, 0u, 0u, 255u, 255u }
    };

    GLuint shader_program;

    {
        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
        glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);

        glCompileShader(vertex_shader);
        glCompileShader(fragment_shader);

        int success;
        char infoLog[512]{};
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertex_shader, 512, nullptr, infoLog);
            SDL_Log("OpenGL: Error compiling vertex shader: %s", infoLog);
            std::exit(EXIT_FAILURE);
        }
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertex_shader, 512, nullptr, infoLog);
            SDL_Log("OpenGL: Error compiling fragment shader: %s", infoLog);
            std::exit(EXIT_FAILURE);
        }

        shader_program = glCreateProgram();

        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, fragment_shader);
        glLinkProgram(shader_program);

        glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
        if (!success) {
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

    glVertexAttribPointer(0, 3, GL_FLOAT,         GL_FALSE, sizeof(Vertex), 0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Vertex), (void*)12);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event) && running) {
            if (event.type == SDL_QUIT) {
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
