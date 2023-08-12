#ifndef ENGINE_STREAM_HPP
#define ENGINE_STREAM_HPP

#include <resources_generated.hpp> // auto-generated

#include <engine/nonnull.hpp>
#include <engine/result.hpp>

#include <cstddef> // std::byte
#include <span> // std::span
#include <string_view> // std::string_view
#include <system_error> // std::errc

namespace engine {
    engine::result<engine::nonnull<resources::BaseResource const>, std::errc> open_resource(std::string_view path) noexcept;
    engine::result<std::span<std::byte const>, std::errc> load_resource(std::string_view path) noexcept;
}

#endif
