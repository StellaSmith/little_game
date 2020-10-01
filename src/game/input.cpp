#include "engine/game.hpp"
#include <SDL.h>

void engine::Game::input(SDL_Event const &event)
{
    if (event.type == SDL_QUIT)
        stop();
    else if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
            glViewport(0, 0, event.window.data1, event.window.data2);
        }
    }
}