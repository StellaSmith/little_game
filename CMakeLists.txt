cmake_minimum_required(VERSION 3.18)
project(little_game)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(WITH_LUA "Which Lua implementation to use [lua, luajit]" "lua")
option(WITH_OPENGL "Use OpenGL as the graphics API" ON)
option(WITH_VULKAN "Use Vulkan as the graphics API" ON)

find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    get_property(PREV_RULE_LAUNCH_COMPILE GLOBAL PROPERTY RULE_LAUNCH_COMPILE)
    get_property(PREV_RULE_LAUNCH_LINK GLOBAL PROPERTY RULE_LAUNCH_LINK)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM} ${PREV_RULE_LAUNCH_COMPILE}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM} ${PREV_RULE_LAUNCH_LINK}")
endif()

find_program(MOLD_PROGRAM mold)

if(MOLD_PROGRAM)
    message(STATUS "Found mold: ${MOLD_PROGRAM}")
    get_property(PREV_RULE_LAUNCH_LINK GLOBAL PROPERTY RULE_LAUNCH_LINK)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${MOLD_PROGRAM} -run ${PREV_RULE_LAUNCH_LINK}")
endif()

if(WITH_LUA STREQUAL "lua")
    find_package(lua REQUIRED)
elseif(WITH_LUA STREQUAL "luajit")
    find_package(luajit REQUIRED)
endif()

if(WITH_OPENGL)
    find_package(glad REQUIRED)
endif()

if(WITH_VULKAN)
    find_package(VulkanHeaders REQUIRED)
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

add_subdirectory(external)

add_executable(little_game)

if(MSVC)
    target_compile_options(little_game PRIVATE "/W4")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang*")
    target_link_options(little_game PRIVATE "-Wl,--as-needed")
    target_compile_options(little_game PRIVATE "-flto" "-Wl,-O1" "-Wl,--as-needed")
    target_compile_options(little_game PRIVATE "-Werror=all" "-Werror=extra" "-Weverything")
else() # assume GCC style compiler
    target_link_options(little_game PRIVATE "-Wl,--as-needed")
    target_compile_options(little_game PRIVATE "-flto" "-Wl,-O1" "-Wl,--as-needed")
    target_compile_options(little_game PRIVATE "-Werror=all" "-Werror=extra" "-Wpedantic")
endif()

file(GLOB_RECURSE VGAME_PRIVATE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*")
file(GLOB_RECURSE VGAME_PUBLIC_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/include/*")
file(GLOB_RECURSE VGAME_OPENGL_PRIVATE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/engine/rendering/opengl/*")
file(GLOB_RECURSE VGAME_OPENGL_PUBLIC_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/include/engine/rendering/opengl/*")
file(GLOB_RECURSE VGAME_VULKAN_PRIVATE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/engine/rendering/vulkan/*")
file(GLOB_RECURSE VGAME_VULKAN_PUBLIC_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/include/engine/rendering/vulkan/*")

list(REMOVE_ITEM VGAME_PRIVATE_SOURCES ${VGAME_OPENGL_PRIVATE_SOURCES} ${VGAME_VULKAN_PRIVATE_SOURCES})
list(REMOVE_ITEM VGAME_PUBLIC_SOURCES ${VGAME_OPENGL_PUBLIC_SOURCES} ${VGAME_VULKAN_PUBLIC_SOURCES})

target_sources(little_game
    PRIVATE
    ${VGAME_PRIVATE_SOURCES}

    PUBLIC
    ${VGAME_PUBLIC_SOURCES}
)

if(WITH_OPENGL)
    target_sources(little_game
        PRIVATE
        ${VGAME_OPENGL_PRIVATE_SOURCES}
        PUBLIC
        ${VGAME_PUBLIC_SOURCES})

    target_link_libraries(little_game
        PRIVATE
        glad::glad
        imgui_impl_opengl3)

    target_compile_definitions(little_game
        PRIVATE
        ENGINE_WITH_OPENGL)
endif()

if(WITH_VULKAN)
    target_sources(little_game PRIVATE
        PRIVATE
        ${VGAME_PUBLIC_PRIVATE_SOURCES}
        PUBLIC
        ${VGAME_PUBLIC_SOURCES})

    target_link_libraries(little_game
        PRIVATE
        Vulkan::Headers
        imgui_impl_vulkan)

    target_compile_definitions(little_game
        PRIVATE
        ENGINE_WITH_VULKAN
        VK_NO_PROTOTYPES
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
endif()

find_program(RESOURCE_COMPILER resource_compiler.py PATHS "${CMAKE_CURRENT_LIST_DIR}/tools/bin" REQUIRED NO_DEFAULT_PATH)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/resources_generated.hpp"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/include/"
    COMMAND "${RESOURCE_COMPILER}" "${CMAKE_CURRENT_LIST_DIR}/res" -o "${CMAKE_CURRENT_BINARY_DIR}/include/resources_generated.hpp"
    DEPENDS "${CMAKE_CURRENT_LIST_DIR}/res"
    VERBATIM
)

target_sources(little_game PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/resources_generated.hpp)
target_include_directories(little_game PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/)

target_link_libraries(little_game
    PRIVATE
    resources
    SDL2::SDL2 glm::glm stb_utils stb_image imgui_impl_sdl2 rapidjson
    fmt::fmt spdlog::spdlog EnTT::EnTT sol2::sol2 ctre::ctre Boost::boost)

if(WITH_LUA STREQUAL "lua")
    target_link_libraries(little_game PRIVATE lua::lua)
elseif(WITH_LUA STREQUAL "luajit")
    target_link_libraries(little_game PRIVATE luajit::luajit)
    target_compile_definitions(little_game PRIVATE SOL_LUAJIT)
endif()

set_target_properties(little_game PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_compile_features(little_game PUBLIC cxx_std_20)
target_include_directories(little_game PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include")
target_compile_definitions(little_game
    PUBLIC
    GLM_FORCE_XYZW_ONLY
    SOL_ALL_SAFETIES_ON)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND NOT(MINGW OR CYGWIN))
    message(STATUS "Linking with SDL2main")
    target_link_libraries(little_game PRIVATE SDL2::SDL2main)
else()
    target_compile_definitions(little_game PRIVATE SDL_MAIN_HANDLED)
endif()

if(UNIX AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/opt/${PROJECT_NAME}" CACHE PATH "" FORCE)
endif()

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set_target_properties(little_game PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
endif()

include(GNUInstallDirs)
install(TARGETS little_game
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
