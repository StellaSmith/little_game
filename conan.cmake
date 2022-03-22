
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.17.0/conan.cmake"
              "${CMAKE_BINARY_DIR}/conan.cmake"
              EXPECTED_HASH SHA512=ef6461fcc8afa6b946daf8cae5a5485bcf1bab23f66a0f2f469edda485d6120c88f02ea8af36f64d885f486a05d5fcab73953a58f3ac0a8d8e64d86c9ebdaeb4
              TLS_VERIFY ON)
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_autodetect(settings)

if (CMAKE_SYSTEM_NAME AND NOT CMAKE_SYSTEM_NAME STREQUAL CMAKE_HOST_SYSTEM_NAME)
    list(APPEND settings "os=${CMAKE_SYSTEM_NAME}")
endif()

if (CMAKE_SYSTEM_PROCESSOR AND NOT CMAKE_SYSTEM_PROCESSOR STREQUAL CMAKE_HOST_SYSTEM_PROCESSOR)
    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "i686")
        list(APPEND settings "arch=x86")
    else()
        list(APPEND settings "arch=${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

conan_cmake_install(
    PATH_OR_REFERENCE ${CMAKE_CURRENT_SOURCE_DIR}
    UPDATE
    BUILD missing
    REMOTE conancenter
    GENERATOR cmake_find_package cmake_paths
    SETTINGS ${settings})

set(CONAN_CMAKE_SILENT_OUTPUT ON)
include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

add_custom_target(conan_check ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/conanfile.txt
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/conanfile.txt
)