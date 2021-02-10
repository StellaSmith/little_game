message(STATUS "Setting up GLAD")

add_library(glad STATIC)
target_sources(glad
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/gl.c
    ${CMAKE_CURRENT_LIST_DIR}/vulkan-hack.c # includes src/vulkan.c
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include/glad/gl.h
    ${CMAKE_CURRENT_LIST_DIR}/include/glad/vulkan.h
    ${CMAKE_CURRENT_LIST_DIR}/include/KHR/khrplatform.h
    ${CMAKE_CURRENT_LIST_DIR}/include/vk_platform.h
)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)


if(WIN32)
    target_compile_definitions(glad PUBLIC VK_USE_PLATFORM_WIN32_KHR)
elseif(NOT (ANDROID OR IOS))
    # as far as i know, X11 is supported in Linux, Most BSD and even MacOS
    target_compile_definitions(glad PUBLIC
        VK_USE_PLATFORM_XLIB_KHR
        VK_USE_PLATFORM_XCB_KHR
        VK_USE_PLATFORM_XLIB_XRANDR_EXT)
endif()

if(ANDROID)
    message(FATAL_ERROR "Android not supported")
    target_compile_definitions(glad PUBLIC 
        VK_USE_PLATFORM_ANDROID_KHR)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_compile_definitions(glad PUBLIC 
        VK_USE_PLATFORM_WAYLAND_KHR
        VK_USE_PLATFORM_DIRECTFB_EXT)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message(WARNING "Some types might be missing")
    target_compile_definitions(glad PUBLIC
        VK_USE_PLATFORM_METAL_EXT
        VK_USE_PLATFORM_MACOS_MVK)
endif()

if (IOS)
    message(FATAL_ERROR "iOS not supported")
    target_compile_definitions(glad PUBLIC 
        VK_USE_PLATFORM_IOS_MVK)
endif()

# MISSING:
#   VK_USE_PLATFORM_FUCHSIA
#   VK_USE_PLATFORM_GGP
#   VK_USE_PLATFORM_VI_NN