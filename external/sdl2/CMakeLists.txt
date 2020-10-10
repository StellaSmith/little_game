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
    add_subdirectory(${sdl2_SOURCE_DIR} ${sdl2_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


# # these are mostly to reduce compile time, since we wont use them
# set(SDL_SHARED OFF CACHE BOOL "" FORCE)
# set(SDL_ATOMIC OFF CACHE BOOL "" FORCE)
# set(SDL_FILESYSTEM OFF CACHE BOOL "" FORCE)
# set(SDL_TEST OFF CACHE BOOL "" FORCE)
# set(SDL_TIMERS OFF CACHE BOOL "" FORCE)
# # needed for audio
# # set(SDL_THREADS OFF CACHE BOOL "" FORCE) 

# if(APPLE)
# set(SDL_FILE ON CACHE BOOL "" FORCE)
# else()
# set(SDL_FILE OFF CACHE BOOL "" FORCE)
# endif()

# set(VIDEO_DUMMY OFF CACHE BOOL "" FORCE)
# # Wayland and KMSDRM need OpenGL ES
# # if(LINUX)
# # set(VIDEO_OPENGLES ON CACHE BOOL "" FORCE)
# # endif()
# set(VIDEO_VULKAN OFF CACHE BOOL "" FORCE)

# # I tried using DirectFB, but it wont compile
# # This will rarely be used anyways
# set(VIDEO_DIRECTFB OFF CACHE BOOL "" FORCE)
