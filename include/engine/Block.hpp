#ifndef ENGINE_BLOCK_T_HPP
#define ENGINE_BLOCK_T_HPP

#include <cstdint>

namespace engine {
    enum Sides : std::uint8_t {
        NORTH = 1 << 0, // -z
        SOUTH = 1 << 1, // +z
        EAST = 1 << 2, // +x
        WEST = 1 << 3, // -x
        TOP = 1 << 4, // +y
        BOTTOM = 1 << 5, // -y

        NONE = 0,
        ALL = NORTH | SOUTH | EAST | WEST | TOP | BOTTOM
    };

    struct Block {
        std::uint32_t id = 0;
        std::uint32_t subid = 0;

        union data_t {
            std::uint64_t u64;
            double f64;
            void *ptr;
        } data { 0ull };
    };

} // namespace engine

#endif
