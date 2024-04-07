cmake_minimum_required(VERSION 3.18)
project(little_game)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(WITH_OPENGL "Use OpenGL as the graphics API" ON)
option(WITH_VULKAN "Use Vulkan as the graphics API" ON)

if(WITH_OPENGL)
    find_package(glad CONFIG REQUIRED)
endif()

if(WITH_VULKAN)
    find_package(VulkanHeaders REQUIRED)
endif()

find_package(glm CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)

add_subdirectory(external)

add_executable(little_game)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR(CMAKE_CXX_COMPILER_ID MATCHES "Clang*" AND NOT MSVC))
    target_link_libraries(little_game PRIVATE -static-libgcc -static-libstdc++)
    message(STATUS "Using static libstdc++ & libgcc_s")

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        target_link_libraries(little_game PRIVATE -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic)
        message(STATUS "Using static libwinpthread")
    endif()

    target_link_libraries(little_game PRIVATE -Wl,-as-needed)
endif()

if(MSVC)
    target_compile_options(little_game PRIVATE "/W4")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang*")
    target_compile_options(little_game PRIVATE "-Werror=all" "-Werror=extra" "-Weverything" "-Wno-pedantic")
else() # assume GCC style compiler
    target_compile_options(little_game PRIVATE "-Werror=all" "-Werror=extra")
endif()

file(GLOB_RECURSE VGAME_PRIVATE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*")
file(GLOB_RECURSE VGAME_PUBLIC_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/include/*")
file(GLOB_RECURSE VGAME_OPENGL_PRIVATE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/engine/rendering/opengl/*")
file(GLOB_RECURSE VGAME_OPENGL_PUBLIC_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/include/engine/rendering/opengl/*")
file(GLOB_RECURSE VGAME_VULKAN_PRIVATE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/engine/rendering/vulkan/*")
file(GLOB_RECURSE VGAME_VULKAN_PUBLIC_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/include/engine/rendering/vulkan/*")
file(GLOB_RECURSE VGAME_WIN32_PRIVATE_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/win32/*")
file(GLOB_RECURSE VGAME_UNIX_PRIVATE_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/unix/*")

list(REMOVE_ITEM VGAME_PRIVATE_SOURCES ${VGAME_OPENGL_PRIVATE_SOURCES} ${VGAME_VULKAN_PRIVATE_SOURCES} ${VGAME_WIN32_PRIVATE_SOURCES} ${VGAME_UNIX_PRIVATE_SOURCES})
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
        ${VGAME_OPENGL_PUBLIC_SOURCES})

    target_link_libraries(little_game
        PRIVATE
        glad::glad
        imgui_impl_opengl3)

    target_compile_definitions(little_game
        PRIVATE
        ENGINE_WITH_OPENGL)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_sources(little_game PRIVATE ${VGAME_WIN32_PRIVATE_SOURCES})
else()
    target_sources(little_game PRIVATE ${VGAME_UNIX_PRIVATE_SOURCES})
endif()

if(WITH_VULKAN)
    find_package(Vulkan MODULE)
    find_package(volk CONFIG REQUIRED)

    target_sources(little_game PRIVATE
        PRIVATE
        ${VGAME_VULKAN_PRIVATE_SOURCES}
        PUBLIC
        ${VGAME_VULKAN_PUBLIC_SOURCES})

    target_link_libraries(little_game
        PRIVATE
        Vulkan::Headers
        volk::volk
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
    SDL2::SDL2 glm::glm imgui_impl_sdl2 rapidjson SDL2_image::SDL2_image
    fmt::fmt spdlog::spdlog EnTT::EnTT Boost::boost range-v3::range-v3)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(little_game PRIVATE Boost::stacktrace_backtrace)
endif()

set_target_properties(little_game PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS ON)
target_compile_features(little_game PUBLIC cxx_std_20)
target_include_directories(little_game PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include")
target_compile_definitions(little_game
    PUBLIC
    GLM_FORCE_XYZW_ONLY)

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
