#ifndef ENGINE_RESOURCES_HPP
#define ENGINE_RESOURCES_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <resources.cpp>

namespace engine {
    resources::BaseResource const *open_resource(char const *path /* utf-8 */) noexcept;
}

#endif