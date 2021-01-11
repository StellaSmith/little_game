message(STATUS "Setting up SDL2")

FetchContent_Declare(sdl2
    URL https://www.libsdl.org/release/SDL2-2.0.12.tar.gz
)
set(SDL_STATIC FALSE CACHE BOOL "Build a static version of the library" FORCE)
set(VIDEO_DUMMY OFF CACHE BOOL "" FORCE)
set(VIDEO_DIRECTFB OFF CACHE BOOL "" FORCE)

FetchContent_GetProperties(sdl2)
if(NOT sdl2_POPULATED)
    FetchContent_Populate(sdl2)
    # if(WIN32)
    #     list(APPEND EXTRA_LIBS vcruntime)
    # endif()
    add_subdirectory(${sdl2_SOURCE_DIR} ${sdl2_BINARY_DIR} EXCLUDE_FROM_ALL)
    if(MSVC)
        # MSVC optimizes something to memset, and because SDL2 doesn't
        # links to the C runtime (vcruntime.dll) it fails to link 
        target_link_options(SDL2 PRIVATE "/alternatename:memset=SDL_memset_REAL")
    endif()
endif()
