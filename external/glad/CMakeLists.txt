

add_library(glad)
target_sources(glad
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/glad.c
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include/glad/glad.h
    ${CMAKE_CURRENT_LIST_DIR}/include/KHR/khrplatform.h
)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
