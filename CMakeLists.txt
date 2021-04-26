cmake_minimum_required(VERSION 3.14)
project(little_game)

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

    src/engine/BlockType.cpp

    src/game/shared.cpp
    src/game/input.cpp
    src/game/update.cpp
    src/game/render.cpp

    src/game/blocks/Air.cpp
    src/game/blocks/Colorful.cpp
    src/game/blocks/Stone.cpp
    src/game/blocks/Dirt.cpp
    src/game/blocks/Grass.cpp

    include/engine/game.hpp
    include/engine/Block.hpp
    include/engine/BlockType.hpp
    include/engine/rendering/Mesh.hpp
    include/engine/rendering/Vertex.hpp
    include/glDebug.h
    include/math/fast.hpp
    include/math/bits.hpp
    include/math/constexpr.hpp
    include/utils/timeit.hpp
    include/utils/cache.hpp
    include/utils/file.hpp
    include/engine/textures.hpp
    include/utils/error.hpp
    include/operators.hpp
)

target_link_libraries(little_game PRIVATE glad SDL2 glm stb imgui nlohmann_json fmt spdlog lua EnTT)

target_compile_features(little_game PUBLIC cxx_std_17)
target_include_directories(little_game PRIVATE include)
target_compile_definitions(little_game PUBLIC GLM_FORCE_XYZW_ONLY)

if(WIN32)
    target_link_libraries(little_game PRIVATE SDL2main)
endif()
if (MSVC)
    target_compile_options(little_game PUBLIC /std:c++latest)
    target_compile_definitions(little_game PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

if (ENABLE_WARNINGS)
    message(STATUS "Warnings enabled")
    target_compile_options(little_game PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
            -Wall -Wextra -pedantic>
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4>)
endif()

if(UNIX AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/opt/${PROJECT_NAME}" CACHE PATH "" FORCE)
endif()

install(TARGETS little_game SDL2
    RUNTIME
        DESTINATION bin
    LIBRARY
        DESTINATION lib
    ARCHIVE
        DESTINATION lib/static
)

install(FILES run.sh run.bat
    DESTINATION .
    PERMISSIONS
        OWNER_EXECUTE
        GROUP_EXECUTE
        WORLD_EXECUTE

        OWNER_READ
        GROUP_READ
        WORLD_READ

        OWNER_WRITE
)

install(DIRECTORY assets cfg
    DESTINATION .
)
