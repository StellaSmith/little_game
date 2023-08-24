#ifndef ENGINE_RENDERING_IRENDERER_HPP
#define ENGINE_RENDERING_IRENDERER_HPP

#include <engine/sdl/Window.hpp>

#include <cstdint>

struct SDL_Window;

namespace engine {
    class Game;
}

namespace engine::rendering {

    class IRenderer {
    public:
        IRenderer(Game &game) noexcept
            : m_game(game)
        {
        }

        virtual engine::sdl::Window create_window(char const *title, int x, int y, int w, int h, uint32_t flags) = 0;

        virtual void setup() = 0;
        virtual void update() = 0;
        virtual void render(float delta) = 0;

        virtual void imgui_setup() = 0;
        virtual void imgui_new_frame(engine::sdl::Window &window) = 0;

        virtual ~IRenderer() = default;

        Game &game() noexcept
        {
            return m_game;
        }

    private:
        Game &m_game;
    };

}

#endif