include(FetchContent)

message(STATUS "Downloading and setting up dependencies, please wait.")

add_subdirectory(glad)
add_subdirectory(stb)
add_subdirectory(sdl2)
add_subdirectory(glm)
add_subdirectory(imgui)
add_subdirectory(nlohmann_json)
add_subdirectory(fmt)
add_subdirectory(spdlog)

message(STATUS "All dependencies set up.")
