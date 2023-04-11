
#ifndef ENGINE_WITH_OPENGL
#error "Engine is configured to not use OpenGL but this file was included"
#endif

#ifndef ENGINE_RENDERING_OPENGL_RENDERER_HPP
#define ENGINE_RENDERING_OPENGL_RENDERER_HPP

#include <SDL_video.h>
#include <glad/glad.h>

#include <engine/components/ChunkPosition.hpp>
#include <engine/rendering/IRenderer.hpp>
#include <engine/rendering/opengl/MeshHandle.hpp>

#include <unordered_map>

namespace engine {
    class Game;
}

namespace engine::rendering::opengl {
    class Renderer final : public virtual IRenderer {
    public:
        using IRenderer::IRenderer;

    private:
        SDL_GLContext m_context;
        GLuint m_vao;
        GLuint m_shader;
        struct {
            GLuint projection;
            GLuint view;
        } m_uniforms;

        std::unordered_map<engine::components::ChunkPosition, std::pair<engine::rendering::opengl::MeshHandle, engine::rendering::opengl::MeshHandle>> m_chunk_meshes;

    public:
        SDL_Window *create_window(char const *title, int x, int y, int w, int h, uint32_t flags) override;
        void setup() override;
        void setup_imgui() override;
        void update() override;
        void render(float delta) override;

        ~Renderer() override;

    private:
        void setup_shader();
        void setup_texture();
    };
}

#endif