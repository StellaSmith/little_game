cmake_minimum_required(VERSION 3.10)
project(little_game)

include(FetchContent)

set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)

message(STATUS "Downloading and setting up dependencies, please wait.")

FetchContent_Declare(sdl2
    URL https://www.libsdl.org/release/SDL2-2.0.12.tar.gz
)

FetchContent_Declare(stb
    URL https://github.com/nothings/stb/archive/f54acd4e13430c5122cab4ca657705c84aa61b08.zip
)

FetchContent_Declare(glad
    URL https://github.com/Dav1dde/glad/archive/v0.1.33.tar.gz
)

FetchContent_Declare(glm
    URL https://github.com/g-truc/glm/archive/0.9.9.8.tar.gz
)

FetchContent_Declare(imgui
    URL https://github.com/ocornut/imgui/archive/v1.77.tar.gz
)

set(GLAD_API "gl=3.3" CACHE STRING "" FORCE)
set(GLAD_PROFILE "core" CACHE STRING "" FORCE)
set(GLAD_EXTENSIONS "GL_KHR_debug" CACHE STRING "" FORCE)

# these are mostly to reduce compile time, since we wont use them
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_ATOMIC OFF CACHE BOOL "" FORCE)
set(SDL_FILESYSTEM OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_THREADS OFF CACHE BOOL "" FORCE)
set(SDL_TIMERS OFF CACHE BOOL "" FORCE)

if(APPLE)
set(SDL_FILE ON CACHE BOOL "" FORCE)
else()
set(SDL_FILE OFF CACHE BOOL "" FORCE)
endif()

set(VIDEO_DUMMY OFF CACHE BOOL "" FORCE)
# Wayland and KMSDRM need OpenGL ES
if(NOT LINUX)
set(VIDEO_OPENGLES OFF CACHE BOOL "" FORCE)
endif()
set(VIDEO_VULKAN OFF CACHE BOOL "" FORCE)

# I tried using DirectFB, but it wont compile
# This will rarely be used anyways
set(VIDEO_DIRECTFB OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glad sdl2 glm)

FetchContent_GetProperties(stb)
if (NOT stb_POPULATED)
    FetchContent_Populate(stb)
    add_library(stb)
    target_include_directories(stb PUBLIC ${stb_SOURCE_DIR})
    target_sources(stb
    PRIVATE
    src/stb/image.c
    src/stb/utils.c
    )
endif()

FetchContent_GetProperties(imgui)
if (NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
    add_library(imgui)
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/examples)
    target_sources(imgui
    PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_internal.h
    PUBLIC
    ${imgui_SOURCE_DIR}/imgui.h

    # Bindings
    PRIVATE
    ${imgui_SOURCE_DIR}/examples/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/examples/imgui_impl_sdl.cpp
    PUBLIC
    ${imgui_SOURCE_DIR}/examples/imgui_impl_opengl3.h
    ${imgui_SOURCE_DIR}/examples/imgui_impl_sdl.h
    )
    target_compile_definitions(imgui PRIVATE IMGUI_DISABLE_INCLUDE_IMCONFIG_H)
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
    target_link_libraries(imgui PUBLIC glad SDL2-static)
endif()

message(STATUS "All dependencies set up.")

add_executable(little_game
    src/main.cpp
    src/glDebug.c
    src/fast_math.cpp
    src/game.cpp
    src/chunk_mesh_generation.cpp
    src/config.cpp

    include/engine/chunk_t.hpp
    include/engine/game.hpp
    include/engine/chunk_mesh_generation.hpp
    include/engine/block_t.hpp
    include/engine/rendering/chunk.hpp
    include/engine/rendering/block.hpp
    include/glDebug.h
    include/math/fast.hpp
    include/math/bits.hpp
    include/math/constexpr.hpp
    include/utils/timeit.hpp
    include/Config.hpp
    include/operators.hpp
)
target_link_libraries(little_game glad SDL2-static glm stb imgui)
target_compile_features(little_game PUBLIC cxx_std_17)
target_include_directories(little_game PRIVATE include)

if(WIN32)
target_link_libraries(little_game SDL2main)
endif()
if (MSVC)
target_compile_options(little_game PUBLIC /std:c++latest)
endif()
