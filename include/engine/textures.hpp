#ifndef ENGINE_TEXTURES_HPP
#define ENGINE_TEXTURES_HPP

#include <string>
#include <unordered_map>

namespace engine {

    struct Textures {
        std::unordered_map<std::string, int> name_to_index;
        unsigned width = 0;
        unsigned height = 0;
        unsigned texture2d_array = 0;
    };

    /**
     * @brief Loads textures from assets/texture_pack.json
     */
    Textures load_textures();
} // namespace engine

#endif