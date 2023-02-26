#pragma once

#include <engine/serializable_component.hpp>

namespace engine::components {
    struct Dirty {
    };
} // namespace engine::components

SERIALIZABLE_COMPONENT(engine::components::Dirty)