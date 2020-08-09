
FetchContent_Declare(stb
    URL https://github.com/nothings/stb/archive/b42009b3b9d4ca35bc703f5310eedc74f584be58.tar.gz
)
FetchContent_GetProperties(stb)
if (NOT stb_POPULATED)
    FetchContent_Populate(stb)
    add_library(stb)
    target_include_directories(stb PUBLIC ${stb_SOURCE_DIR})
    target_sources(stb
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/image.c
    ${CMAKE_CURRENT_LIST_DIR}/utils.c
    )
endif()