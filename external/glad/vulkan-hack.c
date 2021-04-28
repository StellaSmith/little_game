

#ifdef VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#else
#include <KHR/khrplatform.h>
// X11
typedef khronos_uint32_t XID;
typedef khronos_uint32_t VisualID;
typedef struct Display Display;
typedef int Window;
// Xrandr
typedef XID RROutput;
// xcb
typedef struct xcb_connection_t xcb_connection_t;
typedef int xcb_window_t;
typedef khronos_uint32_t xcb_visualid_t;
// DirectFB
typedef struct IDirectFB IDirectFB;
typedef struct IDirectFBSurface IDirectFBSurface;
#endif

#include "./src/vulkan.c"