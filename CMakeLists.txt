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

set(GLAD_API "gl=3.3" CACHE STRING "" FORCE)
set(GLAD_PROFILE "core" CACHE STRING "" FORCE)

# these are mostly to reduce compile time, since we wont use them
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_ATOMIC OFF CACHE BOOL "" FORCE)
set(SDL_FILE OFF CACHE BOOL "" FORCE)
set(SDL_FILESYSTEM OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_THREADS OFF CACHE BOOL "" FORCE)
set(SDL_TIMERS OFF CACHE BOOL "" FORCE)

set(VIDEO_DUMMY OFF CACHE BOOL "" FORCE)
set(VIDEO_KMSDRM OFF CACHE BOOL "" FORCE)
set(VIDEO_OPENGLES OFF CACHE BOOL "" FORCE)
set(VIDEO_VULKAN OFF CACHE BOOL "" FORCE)
set(VIDEO_WAYLAND OFF CACHE BOOL "" FORCE)

set(ALSA OFF CACHE BOOL "" FORCE)
set(JACK OFF CACHE BOOL "" FORCE)

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

message(STATUS "All dependencies set up.")

add_executable(little_game
    src/main.cpp
    src/glDebug.c
    src/chunk_renderer.cpp
    src/fast_math.cpp
    include/engine/chunk_t.hpp
    include/operators.hpp
    include/math/fast.hpp
    include/math/constexpr.hpp
)
target_link_libraries(little_game glad SDL2-static glm stb)
target_compile_features(little_game PUBLIC cxx_std_17)
target_include_directories(little_game PRIVATE include)

if(WIN32)
target_link_libraries(little_game SDL2main)
endif()
