#pragma once

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
} // namespace engine
