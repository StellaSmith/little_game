
find_package(SDL2 REQUIRED)
find_package(imgui REQUIRED)

file(COPY ${imgui_RES_DIRS}/bindings/imgui_impl_sdl.cpp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_library(imgui_impl_sdl OBJECT ${CMAKE_CURRENT_BINARY_DIR}/imgui_impl_sdl.cpp)
target_include_directories(imgui_impl_sdl PUBLIC ${imgui_RES_DIRS}/bindings/)
target_link_libraries(imgui_impl_sdl PUBLIC imgui::imgui PRIVATE SDL2::SDL2)

file(COPY ${imgui_RES_DIRS}/bindings/imgui_impl_opengl3.cpp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_library(imgui_impl_opengl3 OBJECT ${CMAKE_CURRENT_BINARY_DIR}/imgui_impl_opengl3.cpp)
target_include_directories(imgui_impl_opengl3 PUBLIC ${imgui_RES_DIRS}/bindings/)
target_link_libraries(imgui_impl_opengl3 PUBLIC imgui::imgui PRIVATE glad)
target_compile_definitions(imgui_impl_opengl3 PUBLIC IMGUI_IMPL_OPENGL_LOADER_CUSTOM=<glad/gl.h>)