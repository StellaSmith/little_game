message(STATUS "Setting up libfmt")

FetchContent_Declare(fmt
    URL https://github.com/fmtlib/fmt/releases/download/7.1.3/fmt-7.1.3.zip
)

FetchContent_MakeAvailable(fmt)
