
FetchContent_Declare(entt
    URL https://github.com/skypjack/entt/archive/refs/tags/v3.5.2.zip
)

FetchContent_GetProperties(entt)
if(NOT entt_POPULATED)
    FetchContent_Populate(entt)
    add_library(EnTT INTERFACE)
    add_library(EnTT::EnTT ALIAS EnTT)
    target_include_directories(EnTT INTERFACE ${entt_SOURCE_DIR}/src)
    target_compile_features(EnTT INTERFACE cxx_std_17)
endif()
