message(STATUS "Setting up spdlog")

FetchContent_Declare(spdlog
    URL https://github.com/gabime/spdlog/archive/v1.8.2.zip
)

set(SPDLOG_FMT_EXTERNAL ON)

FetchContent_MakeAvailable(spdlog)
