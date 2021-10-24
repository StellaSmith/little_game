cmake_minimum_required(VERSION 3.14)
project(little_game)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CONAN_CMAKE_SILENT_OUTPUT ON)
include(${CMAKE_BINARY_DIR}/conan_paths.cmake OPTIONAL)

find_package(lua REQUIRED)
find_package(glm REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(EnTT REQUIRED)
find_package(SDL2 REQUIRED)
find_package(sol2 REQUIRED)
find_package(glad REQUIRED)

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

    src/engine/BlockRegistry.cpp
    src/engine/BlockType.cpp
    src/engine/Config.cpp

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
    include/utils/file.hpp
    include/engine/textures.hpp
    include/utils/error.hpp
    include/operators.hpp

    include/engine/assets/BlockModel.hpp

    include/engine/errors/AlreadyRegistered.hpp
    include/engine/errors/UnsupportedFileType.hpp
)

list(APPEND CXX_PRECOMPILED_HEADERS "string" "string_view" "vector" "type_traits" "chrono" "utility")
list(APPEND CXX_PRECOMPILED_HEADERS "spdlog/spdlog.h" "fmt/format.h" "glm/glm.hpp" "entt/entt.hpp")
foreach(HEADER ${CXX_PRECOMPILED_HEADERS})
    target_precompile_headers(little_game PRIVATE $<$<COMPILE_LANGUAGE:CXX>:<${HEADER}$<ANGLE-R>>)    
endforeach()



target_link_libraries(little_game
PRIVATE
    glad::glad SDL2::SDL2 glm::glm stb_utils stb_image imgui_impl_opengl3 imgui_impl_sdl RapidJSON::RapidJSON
    fmt::fmt spdlog::spdlog EnTT::EnTT sol2::sol2 lua::lua)

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

install(DIRECTORY assets cfg
    DESTINATION .
)
