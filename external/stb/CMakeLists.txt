
find_package(stb REQUIRED)

add_library(stb_image OBJECT ${CMAKE_CURRENT_LIST_DIR}/stb_image.c)
target_link_libraries(stb_image PUBLIC stb::stb)


add_library(stb_utils OBJECT ${CMAKE_CURRENT_LIST_DIR}/stb.c)
target_link_libraries(stb_utils PUBLIC stb::stb)