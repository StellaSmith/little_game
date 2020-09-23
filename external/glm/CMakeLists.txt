message(STATUS "Setting up GLM")

FetchContent_Declare(glm
    URL https://github.com/g-truc/glm/archive/0.9.9.8.tar.gz
)

set(GLM_TEST_ENABLE OFF)

FetchContent_MakeAvailable(glm)