cmake_minimum_required(VERSION 3.10)
project(little_game)

include(FetchContent)

set(GLM_TEST_ENABLE OFF)

message(STATUS "Downloading and setting up dependencies, please wait.")

FetchContent_Declare(sdl2
    URL https://www.libsdl.org/release/SDL2-2.0.12.tar.gz
)

FetchContent_Declare(glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.33
    GIT_PROGRESS ON
    GIT_SHALLOW ON
)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
    GIT_PROGRESS ON
    GIT_SHALLOW ON
)

set(GLAD_API "gl=3.3")
set(GLAD_PROFILE "core")
FetchContent_MakeAvailable(glad sdl2 glm)

message(STATUS "All dependencies set up.")

add_executable(little_game
    src/main.cpp
    src/glDebug.c
)
target_link_libraries(little_game glad SDL2-static glm)
target_compile_features(little_game PUBLIC cxx_std_17)
target_include_directories(little_game PRIVATE include)

if(WIN32)
target_link_libraries(little_game SDL2main)
endif()
