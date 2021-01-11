message(STATUS "Setting up Nlohmann JSON")

FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/archive/v3.9.1.tar.gz
)

set(JSON_BuildTests OFF)
set(JSON_Install OFF)
set(JSON_MultipleHeaders ON)
set(JSON_ImplicitConversions OFF)

FetchContent_MakeAvailable(nlohmann_json)