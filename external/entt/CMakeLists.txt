message(STATUS "Setting up entt")

FetchContent_Declare(entt
    URL https://github.com/skypjack/entt/archive/refs/tags/v3.5.2.zip
)

set(BUILD_TESTING OFF)
FetchContent_MakeAvailable(entt)
