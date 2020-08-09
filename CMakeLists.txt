cmake_minimum_required(VERSION 3.10)
project(little_game)

add_subdirectory(external)

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
