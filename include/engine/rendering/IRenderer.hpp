#ifndef ENGINE_RENDERING_IRENDERER_HPP
#define ENGINE_RENDERING_IRENDERER_HPP

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

        virtual void setup() = 0;
        virtual void update() = 0;
        virtual void render(float delta) = 0;
        virtual SDL_Window *create_window(char const *title, int x, int y, int w, int h, uint32_t flags) = 0;
        virtual void setup_imgui() = 0;
        virtual ~IRenderer() = 0;

        Game &game() noexcept
        {
            return m_game;
        }

    private:
        Game &m_game;
    };

}

#endif