cmake_minimum_required(VERSION 3.0)
project(mold VERSION ${MOLD_VERSION} LANGUAGES CXX)

include(conanbuildinfo.cmake)
conan_basic_setup(TARGETS KEEP_RPATHS)

set(CMAKE_CXX_STANDARD 20)

find_package(TBB REQUIRED)

file(GLOB SOURCE_FILES "source_subfolder/*.cc" "source_subfolder/elf/*.cc"  "source_subfolder/macho/*.cc")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE TBB::TBB)
target_compile_definitions(${PROJECT_NAME} PRIVATE "MOLD_VERSION=\"${MOLD_VERSION}\"")

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})