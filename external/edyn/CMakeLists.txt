message(STATUS "Setting up edyn")

FetchContent_Declare(edyn
    URL https://github.com/xissburg/edyn/archive/520a285548f7739b16dc44bb22b944469b12d296.zip
)

FetchContent_MakeAvailable(edyn)
