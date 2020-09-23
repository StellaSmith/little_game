cmake_minimum_required(VERSION 3.10)
project(little_game)

add_subdirectory(external)


add_executable(little_game
    src/main.cpp
    src/glDebug.c
    src/fast_math.cpp
    src/game.cpp
    src/chunk_mesh_generation.cpp
    src/cache.cpp

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
    include/utils/cache.hpp
    include/operators.hpp
)
target_link_libraries(little_game glad SDL2-static glm stb imgui nlohmann_json)
target_compile_features(little_game PUBLIC cxx_std_17)
target_include_directories(little_game PRIVATE include)
target_compile_definitions(little_game PUBLIC GLM_FORCE_XYZW_ONLY)

if(WIN32)
target_link_libraries(little_game SDL2main)
endif()
if (MSVC)
target_compile_options(little_game PUBLIC /std:c++latest)
endif()

if (ENABLE_WARNINGS)
message(STATUS "Warnings enabled")
target_compile_options(little_game PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Wall -Wextra>
     $<$<CXX_COMPILER_ID:MSVC>:
          /W4>)
endif()
