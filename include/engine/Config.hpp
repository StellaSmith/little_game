#ifndef ENGINE_CONFIG_HPP
#define ENGINE_CONFIG_HPP

#include <filesystem>
#include <optional>
#include <string>

namespace engine {

    struct Config;

    Config const &config();

    struct Config {

        struct {
            std::optional<std::string> video_driver;
            std::optional<std::string> audio_driver;
        } sdl;

        struct {
            std::optional<unsigned> red_bits;
            std::optional<unsigned> green_bits;
            std::optional<unsigned> blue_bits;
            std::optional<unsigned> alpha_bits;
            std::optional<unsigned> depth_bits;
            std::optional<unsigned> stencil_bits;
        } opengl;

        struct {
            std::optional<std::string> font_path;
        } imgui;

        struct {
            std::filesystem::path root = ".";
            std::filesystem::path cache = root / "cache";
        } folders;

        static Config const &load(std::filesystem::path const &);
        static Config const &the() noexcept
        {
            return ::engine::config();
        }
    };
}

#endif
