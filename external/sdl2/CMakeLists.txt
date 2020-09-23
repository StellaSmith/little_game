message(STATUS "Setting up SDL2")

FetchContent_Declare(sdl2
    URL https://www.libsdl.org/release/SDL2-2.0.12.tar.gz
)

set(VIDEO_DUMMY OFF CACHE BOOL "" FORCE)
set(VIDEO_DIRECTFB OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sdl2)


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