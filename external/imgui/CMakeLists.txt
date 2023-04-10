
find_package(SDL2 REQUIRED)
find_package(imgui REQUIRED)


add_library(imgui_impl_sdl2 OBJECT ${imgui_RES_DIRS}/bindings/imgui_impl_sdl2.cpp)
target_include_directories(imgui_impl_sdl2 PUBLIC ${imgui_RES_DIRS}/bindings/)
target_link_libraries(imgui_impl_sdl2 PUBLIC imgui::imgui PRIVATE SDL2::SDL2)

if (WITH_OPENGL)
    find_package(glad REQUIRED)
    add_library(imgui_impl_opengl3 OBJECT ${imgui_RES_DIRS}/bindings/imgui_impl_opengl3.cpp)
    target_include_directories(imgui_impl_opengl3 PUBLIC ${imgui_RES_DIRS}/bindings/)
    target_link_libraries(imgui_impl_opengl3 PUBLIC imgui::imgui PRIVATE glad::glad)
    target_compile_definitions(imgui_impl_opengl3 PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
endif()

if (WITH_VULKAN)
    find_package(volk REQUIRED)
    add_library(imgui_impl_vulkan OBJECT ${imgui_RES_DIRS}/bindings/imgui_impl_vulkan.cpp)
    target_include_directories(imgui_impl_vulkan PUBLIC ${imgui_RES_DIRS}/bindings/)
    target_link_libraries(imgui_impl_vulkan PUBLIC imgui::imgui volk::volk)
    target_compile_definitions(imgui_impl_vulkan PUBLIC VK_NO_PROTOTYPES)
endif()