message(STATUS "Setting up entt")

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

find_package(EnTT REQUIRED)
