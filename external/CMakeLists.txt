include(FetchContent)

message(STATUS "Downloading and setting up dependencies, please wait.")

add_subdirectory(glad)
add_subdirectory(stb)
add_subdirectory(imgui)

message(STATUS "All dependencies set up.")
