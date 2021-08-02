
find_package(SDL2 REQUIRED)
find_package(imgui REQUIRED)
find_package(glad REQUIRED)

add_library(imgui_impl_sdl OBJECT ${imgui_RES_DIRS}/bindings/imgui_impl_sdl.cpp)
target_include_directories(imgui_impl_sdl PUBLIC ${imgui_RES_DIRS}/bindings/)
target_link_libraries(imgui_impl_sdl PUBLIC imgui::imgui PRIVATE SDL2::SDL2)

add_library(imgui_impl_opengl3 OBJECT ${imgui_RES_DIRS}/bindings/imgui_impl_opengl3.cpp)
target_include_directories(imgui_impl_opengl3 PUBLIC ${imgui_RES_DIRS}/bindings/)
target_link_libraries(imgui_impl_opengl3 PUBLIC imgui::imgui PRIVATE glad::glad)
target_compile_definitions(imgui_impl_opengl3 PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)