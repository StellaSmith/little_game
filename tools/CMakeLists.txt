cmake_minimum_required(VERSION 3.16)
project(vengine_tools)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(USE_CONAN "Use the Conan C/C++ package manager to automatically gather dependencies" ON)

if(USE_CONAN)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../conan.cmake)
endif()

function(add_simple_tool tool_name)
    add_executable(${tool_name})
    target_sources(${tool_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/${tool_name}.cpp)
    target_compile_features(${tool_name} PRIVATE cxx_std_20)

    foreach(dep ${ARGN})
        string(REGEX MATCH "([a-zA-Z0-9]+)::([a-zA-Z0-9]+)" dep_match ${dep})
        find_package(${CMAKE_MATCH_1} REQUIRED)
        target_link_libraries(${tool_name} PRIVATE ${dep})
    endforeach()
    
    install(TARGETS ${tool_name} RUNTIME DESTINATION bin)
endfunction()

add_simple_tool(resource_compiler Boost::Boost argparse::argparse fmt::fmt RapidJSON::RapidJSON)
add_simple_tool(install_shared_libraries argparse::argparse fmt::fmt ctre::ctre)