
find_package(SDL2 REQUIRED CONFIG)
find_package(imgui REQUIRED CONFIG)

if(NOT DEFINED CACHE{IMGUI_RES_DIR})
    message(FATAL_ERROR "IMGUI_RES_DIR is not defined")
endif()

add_library(imgui_impl_sdl2 OBJECT)
target_sources(imgui_impl_sdl2
    PRIVATE "${IMGUI_RES_DIR}/bindings/imgui_impl_sdl2.cpp"
    PUBLIC "${IMGUI_RES_DIR}/bindings/imgui_impl_sdl2.h")
target_include_directories(imgui_impl_sdl2 PUBLIC "${IMGUI_RES_DIR}/bindings/")
target_link_libraries(imgui_impl_sdl2 PUBLIC imgui::imgui SDL2::SDL2)

if(WITH_OPENGL)
    find_package(glad REQUIRED)
    add_library(imgui_impl_opengl3 OBJECT)
    target_sources(imgui_impl_opengl3
        PRIVATE "${IMGUI_RES_DIR}/bindings/imgui_impl_opengl3.cpp"
        PUBLIC "${IMGUI_RES_DIR}/bindings/imgui_impl_opengl3.h")
    target_include_directories(imgui_impl_opengl3 PUBLIC "${IMGUI_RES_DIR}/bindings/")
    target_link_libraries(imgui_impl_opengl3 PUBLIC imgui::imgui PRIVATE glad::glad)
    target_compile_definitions(imgui_impl_opengl3 PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
endif()

if(WITH_VULKAN)
    find_package(Vulkan REQUIRED MODULE)
    add_library(imgui_impl_vulkan OBJECT)
    target_sources(imgui_impl_vulkan
        PRIVATE "${IMGUI_RES_DIR}/bindings/imgui_impl_vulkan.cpp"
        PUBLIC "${IMGUI_RES_DIR}/bindings/imgui_impl_vulkan.h")
    target_include_directories(imgui_impl_vulkan PUBLIC "${IMGUI_RES_DIR}/bindings/")
    target_link_libraries(imgui_impl_vulkan PUBLIC imgui::imgui Vulkan::Headers)
    target_compile_definitions(imgui_impl_vulkan PUBLIC VK_NO_PROTOTYPES)
endif()