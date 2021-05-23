
find_package(stb REQUIRED)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/stb_image.c "#define STB_IMAGE_IMPLEMENTATION\n#include <stb_image.h>")
add_library(stb_image OBJECT ${CMAKE_CURRENT_BINARY_DIR}/stb_image.c)
target_link_libraries(stb_image PUBLIC stb::stb)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/stb_utils.c "#define STB_DEFINE\n#include <stb.h>")
add_library(stb_utils OBJECT ${CMAKE_CURRENT_BINARY_DIR}/stb_utils.c)
target_link_libraries(stb_utils PUBLIC stb::stb)