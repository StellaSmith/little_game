message(STATUS "Setting up edyn")

FetchContent_Declare(edyn
    URL https://github.com/xissburg/edyn/archive/f7ec7031fc4a644264d62b632ade2e39a002483b.zip
)

FetchContent_MakeAvailable(edyn)
