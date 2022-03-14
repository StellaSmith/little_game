cmake_minimum_required(VERSION 3.14)
project(little_game)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "ccache: Found program ${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()


find_program(MOLD_PROGRAM mold)
if(MOLD_PROGRAM AND NOT CMAKE_SYSTEM_NAME STREQUAL Windows)
    message(STATUS "mold: Found program ${MOLD_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM} ${MOLD_PROGRAM} -run")
endif()


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(USE_CONAN "Use the Conan C/C++ package manager to automatically gather dependencies" ON)
option(ENABLE_WARNINGS "Use as many warnings as possible" ON)

if(USE_CONAN)
    include(${CMAKE_CURRENT_SOURCE_DIR}/conan.cmake)
endif()

if (ENABLE_WARNINGS)
    message(STATUS "Warnings enabled")
    if (MSVC)
        add_compile_options(/W4)
    else()
        add_compile_options(-Wall -Wextra -pedantic)
    endif()
endif()

find_package(lua REQUIRED)
find_package(glm REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(EnTT REQUIRED)
find_package(SDL2 REQUIRED)
find_package(sol2 REQUIRED)
find_package(glad REQUIRED)
find_package(ctre REQUIRED)
find_package(absl REQUIRED)

add_subdirectory(external)

add_executable(little_game
    src/main.cpp
    src/glDebug.c
    src/fast_math.cpp
    src/chunk_mesh_generation.cpp
    src/cache.cpp
    src/file.cpp
    src/textures.cpp
    src/error.cpp
    src/lua_bindings.cpp

    src/engine/Config.cpp
    src/engine/resources.cpp

    src/game/shared.cpp
    src/game/input.cpp
    src/game/update.cpp
    src/game/render.cpp

    src/engine/errors/AlreadyRegistered.cpp
    src/engine/errors/UnsupportedFileType.cpp


    # src/game/blocks/Air.cpp
    # src/game/blocks/Colorful.cpp
    # src/game/blocks/Stone.cpp
    # src/game/blocks/Dirt.cpp
    # src/game/blocks/Grass.cpp

    src/assets/BlockModel.cpp

    include/engine/Game.hpp
    include/engine/Block.hpp
    include/engine/BlockType.hpp
    include/engine/rendering/Mesh.hpp
    include/engine/rendering/Vertex.hpp
    include/engine/Config.hpp
    include/glDebug.h
    include/math/fast.hpp
    include/math/bits.hpp
    include/math/constexpr.hpp
    include/utils/timeit.hpp
    include/utils/cache.hpp
    include/utils/strings.hpp
    include/utils/file.hpp
    include/engine/textures.hpp
    include/utils/error.hpp
    include/operators.hpp
    include/utils/url.hpp
    include/engine/Stream.hpp

    include/engine/assets/BlockModel.hpp

    include/engine/errors/AlreadyRegistered.hpp
    include/engine/errors/UnsupportedFileType.hpp
)

# list(APPEND CXX_PRECOMPILED_HEADERS "string" "string_view" "vector" "type_traits" "chrono" "utility")
# list(APPEND CXX_PRECOMPILED_HEADERS "spdlog/spdlog.h" "fmt/format.h" "glm/glm.hpp" "entt/entt.hpp")
# foreach(HEADER ${CXX_PRECOMPILED_HEADERS})
#     target_precompile_headers(little_game PRIVATE $<$<COMPILE_LANGUAGE:CXX>:<${HEADER}$<ANGLE-R>>)    
# endforeach()

find_package(Python 3 REQUIRED COMPONENTS Interpreter)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/
    COMMAND  ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/include/
)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/resources.cpp
    COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_LIST_DIR}/tools/resource_compiler.py" > ${CMAKE_CURRENT_BINARY_DIR}/include/resources.cpp
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/include/ "${CMAKE_CURRENT_LIST_DIR}/tools/resource_compiler.py" "${CMAKE_CURRENT_LIST_DIR}/res"
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/res"
    USES_TERMINAL
)
add_library(resources OBJECT ${CMAKE_CURRENT_BINARY_DIR}/include/resources.cpp)
target_compile_features(resources PRIVATE cxx_std_17)
target_compile_definitions(resources PRIVATE COMPILE_RESOURCES)
target_include_directories(resources PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include/)

target_link_libraries(little_game
PRIVATE
    resources
    glad::glad SDL2::SDL2 glm::glm stb_utils stb_image imgui_impl_opengl3 imgui_impl_sdl RapidJSON::RapidJSON
    fmt::fmt spdlog::spdlog EnTT::EnTT sol2::sol2 lua::lua ctre::ctre absl::absl)

set_target_properties(little_game PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_compile_features(little_game PUBLIC cxx_std_17)
target_include_directories(little_game PRIVATE include)
target_compile_definitions(little_game PUBLIC
    GLM_FORCE_XYZW_ONLY
    # SOL_LUAJIT
    SOL_ALL_SAFETIES_ON)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND NOT (MINGW OR CYGWIN))
    message(STATUS "Linking with SDL2main")
    target_link_libraries(little_game PRIVATE SDL2::SDL2main)
else()
    target_compile_definitions(little_game PRIVATE SDL_MAIN_HANDLED)
endif()
if (MSVC)
    target_compile_options(little_game PUBLIC /std:c++latest)
    target_compile_definitions(little_game PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

if(UNIX AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/opt/${PROJECT_NAME}" CACHE PATH "" FORCE)
endif()

if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set_target_properties(little_game PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
endif()
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

install(TARGETS little_game
    RUNTIME
        DESTINATION bin
    LIBRARY
        DESTINATION lib
    ARCHIVE
        DESTINATION lib/static
)

install(DIRECTORY assets cfg lua
    DESTINATION .
)
