include(FetchContent)

message(STATUS "Downloading and setting up dependencies, please wait.")

add_subdirectory(glad)
add_subdirectory(stb)

FetchContent_Declare(sdl2
    URL https://www.libsdl.org/release/SDL2-2.0.12.tar.gz
)

FetchContent_Declare(glm
    URL https://github.com/g-truc/glm/archive/0.9.9.8.tar.gz
)

FetchContent_Declare(imgui
    URL https://github.com/ocornut/imgui/archive/v1.77.tar.gz
)

FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/archive/v3.9.1.tar.gz
)

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
# if(LINUX)
# set(VIDEO_OPENGLES ON CACHE BOOL "" FORCE)
# endif()
set(VIDEO_VULKAN OFF CACHE BOOL "" FORCE)

# I tried using DirectFB, but it wont compile
# This will rarely be used anyways
set(VIDEO_DIRECTFB OFF CACHE BOOL "" FORCE)


set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
set(JSON_MultipleHeaders ON CACHE BOOL "" FORCE)
set(JSON_ImplicitConversions OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sdl2 glm nlohmann_json)


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
