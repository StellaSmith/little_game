#ifndef ENGINE_CONFIG_HPP
#define ENGINE_CONFIG_HPP

#include <string_view>

namespace engine {
    struct Config {

        struct {
            std::string_view video_driver = {};
            std::string_view audio_driver = {};
        } sdl;

        struct {
            int red_bits = -1;
            int green_bits = -1;
            int blue_bits = -1;
            int alpha_bits = -1;
            int depth_bits = -1;
            int stencil_bits = -1;
        } opengl;

        struct {
            std::string_view font_path;
        } imgui;

        struct {
            int max_lines = 5000;
        } terminal;
    };

    Config const &get_engine_config();
    Config const &load_engine_config();
}

#endif
