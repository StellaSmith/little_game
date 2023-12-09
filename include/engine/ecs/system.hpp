#pragma once

#include <entt/entt.hpp>
#include <span>

namespace engine::ecs {
    struct System {
        entt::hashed_string name;
        void (*pfn_execute)(entt::registry &registry);
    };

    struct SystemDependencies {
        entt::hashed_string name;
        std::span<entt::hashed_string const> dependencies;
    };

    template <typename T>
    System make_system(entt::hashed_string name, T func)
    {
        return System {
            .name = name,
            .pfn_execute = func,
        };
    }

    template <typename With, typename Without>
    class View;

    template <typename... With, typename... Without>
    class View<entt::get_t<With...>, entt::exclude_t<Without...>> {
        template <typename Entity, typename Allocator>
        static decltype(auto) get(entt::basic_registry<Entity, Allocator> &registry)
        {
            return registry.template view<With...>(entt::exclude<Without...>);
        }
    };

    template <typename T>
    class Res {
        template <typename Entity, typename Allocator>
        static decltype(auto) get(entt::basic_registry<Entity, Allocator> &registry)
        {
            return registry.ctx().template get<T>();
        }
    };

    extern std::span<System const> const systems;
    extern std::span<SystemDependencies const> const systems_dependencies;
} // namespace engine::ecs

#define CONCAT(a, b) a##b
#define ECS_SYSTEM(name)                                                                                                           \
    namespace {                                                                                                                    \
        __attribute__((section("system_array"), used)) static auto CONCAT(name, _system) = engine::ecs::make_system(#name, &name); \
    }
#define ECS_SYSTEM_DEPENDS(name, ...)                                                                                                                                                  \
    namespace {                                                                                                                                                                        \
        __attribute__((section("system_dependencies_array"), used)) static auto const CONCAT(name, _system_dependencies) = engine::ecs::SystemDependencies { #name, { __VA_ARGS__ } }; \
    }
