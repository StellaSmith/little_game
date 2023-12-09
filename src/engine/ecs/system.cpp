#include <engine/ecs/system.hpp>

// these arrays are populated at link time thanks to the macros which use the section attribute
extern "C" engine::ecs::System const __start_system_array[];
extern "C" engine::ecs::System const __stop_system_array[];
extern "C" engine::ecs::SystemDependencies const __start_system_dependencies_array[];
extern "C" engine::ecs::SystemDependencies const __stop_system_dependencies_array[];

std::span<const engine::ecs::System> const engine::ecs::systems = std::span<engine::ecs::System const>(
    &__start_system_array[0],
    &__stop_system_array[0]);

std::span<engine::ecs::SystemDependencies const> const engine::ecs::systems_dependencies = std::span<engine::ecs::SystemDependencies const>(
    &__start_system_dependencies_array[0],
    &__stop_system_dependencies_array[0]);