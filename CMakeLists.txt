cmake_minimum_required(VERSION 3.14)
project(little_game)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(USE_CONAN   "Use the Conan C/C++ package manager" OFF)
option(WITH_LUAJIT "Use LuaJIT instead of Lua" ON)
option(WITH_OPENGL "Use OpenGL as the graphics API" ON)
option(WITH_VULKAN "Use Vulkan as the graphics API" ON)

if (USE_CONAN)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS KEEP_RPATHS)
endif()

find_program(MOLD_PROGRAM mold)
if(MOLD_PROGRAM AND NOT CMAKE_SYSTEM_NAME STREQUAL Windows)
    message(STATUS "mold: Found program ${MOLD_PROGRAM}")
    get_property(PREV_RULE_LAUNCH_LINK GLOBAL PROPERTY RULE_LAUNCH_LINK)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${MOLD_PROGRAM} -run ${PREV_RULE_LAUNCH_LINK}")
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "ccache: Found program ${CCACHE_PROGRAM}")
    get_property(PREV_RULE_LAUNCH_COMPILE GLOBAL PROPERTY RULE_LAUNCH_COMPILE)
    get_property(PREV_RULE_LAUNCH_LINK GLOBAL PROPERTY RULE_LAUNCH_LINK)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM} ${PREV_RULE_LAUNCH_COMPILE}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM} ${PREV_RULE_LAUNCH_LINK}")
endif()

if (MSVC)
    set_property(GLOBAL APPEND PROPERTY COMPILE_FLAGS "/W4")
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang*")
    set_property(GLOBAL APPEND PROPERTY LINK_FLAGS "-Wl,--as-needed")
    set_property(GLOBAL APPEND PROPERTY COMPILE_FLAGS "-flto" "-Wl,--as-needed")
    set_property(GLOBAL APPEND PROPERTY COMPILE_FLAGS "-Weverything" "-Wpedantic-errors")
else()
    set_property(GLOBAL APPEND PROPERTY LINK_FLAGS "-Wl,--as-needed")
    set_property(GLOBAL APPEND PROPERTY COMPILE_FLAGS "-flto" "-Wl,--as-needed")
    set_property(GLOBAL APPEND PROPERTY COMPILE_FLAGS "-Wall" "-Wextra" "-Wpedantic-errors")
endif()

if (WITH_LUAJIT)
    find_package(luajit REQUIRED)
else()
    find_package(lua REQUIRED)
endif()

if (WITH_OPENGL)
    find_package(glad REQUIRED)
endif()

if (WITH_VULKAN)
    find_package(Vulkan REQUIRED)
    find_package(volk REQUIRED)
endif()

find_package(glm REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(EnTT REQUIRED)
find_package(SDL2 REQUIRED)
find_package(sol2 REQUIRED)
find_package(ctre REQUIRED)
find_package(Boost REQUIRED)
find_package(TBB REQUIRED)

add_subdirectory(external)

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(aligned_alloc stdlib.h HAS_C11_ALIGNED_ALLOC)
check_cxx_symbol_exists(posix_memalign stdlib.h HAS_POSIX_MEMALIGN)
check_cxx_symbol_exists(_aligned_malloc malloc.h HAS_WIN32_ALIGNED_MALLOC)

add_executable(little_game
    src/main.cpp
    src/fast_math.cpp
    src/chunk_mesh_generation.cpp
    src/cache.cpp
    src/file.cpp
    src/textures.cpp
    src/error.cpp
    src/lua_bindings.cpp
    src/lua/format.cpp

    src/engine/Config.cpp
    src/engine/resources.cpp

    src/game/shared.cpp
    src/game/input.cpp
    src/game/update.cpp

    src/engine/errors/AlreadyRegistered.cpp
    src/engine/errors/UnsupportedFileType.cpp
    src/engine/errors/InvalidResourceName.cpp


    # src/game/blocks/Air.cpp
    # src/game/blocks/Colorful.cpp
    # src/game/blocks/Stone.cpp
    # src/game/blocks/Dirt.cpp
    # src/game/blocks/Grass.cpp

    src/assets/BlockMesh.cpp
    src/assets/Image.cpp

    include/engine/Game.hpp
    include/engine/Block.hpp
    include/engine/BlockType.hpp
    include/engine/rendering/Mesh.hpp
    include/engine/rendering/Vertex.hpp
    include/engine/Config.hpp
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

    include/engine/assets/BlockMesh.hpp
    include/engine/assets/Image.hpp

    include/engine/errors/AlreadyRegistered.hpp
    include/engine/errors/UnsupportedFileType.hpp
    include/engine/errors/InvalidResourceName.hpp
)

if (WITH_OPENGL)
    target_sources(little_game PRIVATE
        src/glDebug.c
        src/engine/rendering/opengl/render.cpp

        include/glDebug.h
    )
endif()

if (WITH_VULKAN)
    target_sources(little_game PRIVATE
        src/engine/rendering/vulkan/render.cpp
    )
endif()

find_program(RESOURCE_COMPILER resource_compiler REQUIRED)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/resources.cpp
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/include/
    COMMAND ${RESOURCE_COMPILER} "${CMAKE_CURRENT_LIST_DIR}/res" -o "${CMAKE_CURRENT_BINARY_DIR}/include/resources.cpp"
    DEPENDS "${CMAKE_CURRENT_LIST_DIR}/res"
    VERBATIM
)
add_library(resources OBJECT ${CMAKE_CURRENT_BINARY_DIR}/include/resources.cpp)
target_compile_features(resources PRIVATE cxx_std_20)
target_compile_definitions(resources PRIVATE COMPILE_RESOURCES)
target_include_directories(resources PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include/)

target_link_libraries(little_game
PRIVATE
    resources
    SDL2::SDL2 glm::glm stb_utils stb_image imgui_impl_sdl RapidJSON::RapidJSON
    fmt::fmt spdlog::spdlog EnTT::EnTT sol2::sol2 ctre::ctre Boost::Boost TBB::tbbmalloc)

if (WITH_OPENGL)
    target_link_libraries(little_game PRIVATE
        glad::glad
        imgui_impl_opengl3)
endif()

if (WITH_VULKAN)
    target_link_libraries(little_game PRIVATE
        Vulkan::Headers
        volk::volk
        imgui_impl_vulkan)
endif()

if (WITH_LUAJIT)
    target_link_libraries(little_game PRIVATE luajit::luajit)
else()
    target_link_libraries(little_game PRIVATE lua::lua)
endif()

set_target_properties(little_game PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_compile_features(little_game PUBLIC cxx_std_20)
target_include_directories(little_game PRIVATE include)
target_compile_definitions(little_game PUBLIC
    GLM_FORCE_XYZW_ONLY
    SOL_ALL_SAFETIES_ON
    $<$<BOOL:HAS_C11_ALIGNED_ALLOC>:HAS_C11_ALIGNED_ALLOC>
    $<$<BOOL:HAS_POSIX_MEMALIGN>:HAS_POSIX_MEMALIGN>
    $<$<BOOL:HAS_WIN32_ALIGNED_MALLOC>:HAS_WIN32_ALIGNED_MALLOC>)

if (WITH_LUAJIT)
    target_compile_definitions(little_game PUBLIC SOL_LUAJIT)
endif()

if (WITH_OPENGL)
    target_compile_definitions(little_game PUBLIC ENGINE_WITH_OPENGL)
endif()

if (WITH_VULKAN)
    target_compile_definitions(little_game PUBLIC ENGINE_WITH_VULKAN VK_NO_PROTOTYPES VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
endif()

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

include(GNUInstallDirs)
install(TARGETS little_game
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
