
FetchContent_Declare(imgui
    URL https://github.com/ocornut/imgui/archive/v1.77.tar.gz
)

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
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_CUSTOM=<glad/gl.h>)
    target_link_libraries(imgui PUBLIC glad SDL2)
endif()
