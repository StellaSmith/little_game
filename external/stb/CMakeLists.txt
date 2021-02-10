message(STATUS "Setting up stb")

FetchContent_Declare(stb
    URL https://github.com/StellaSmith/stb/archive/78625bbb7c7af00e59b5a52484fa9ec0168dd1cf.zip
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
