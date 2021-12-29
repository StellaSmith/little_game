
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
              "${CMAKE_BINARY_DIR}/conan.cmake"
              EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
              TLS_VERIFY ON)
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_autodetect(settings)

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