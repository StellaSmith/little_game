#include <engine/rendering/vulkan/State.hpp>

#if defined(HAS_C11_ALIGNED_ALLOC) || defined(HAS_POSIX_MEMALIGN)
#include <stdlib.h>
#endif

#ifdef HAS_WIN32_ALIGNED_MALLOC
#include <malloc.h>
#endif

#include <fmt/core.h>
#include <oneapi/tbb/scalable_allocator.h>

engine::rendering::VulkanState engine::rendering::VulkanState::with_malloc() noexcept
{
    VulkanState result;

    result.m_allocationCallbacks = VkAllocationCallbacks {
        .pfnAllocation = [](void *, size_t bytes, size_t alignment, VkSystemAllocationScope) -> void * {
#if defined(HAS_C11_ALIGNED_ALLOC)
            return aligned_alloc(alignment, bytes);
#elif defined(HAS_POSIX_MEMALIGN)
            void *ptr = nullptr;
            posix_memalign(&ptr, alignment, bytes);
            return ptr;
#elif defined(HAS_WIN32_ALIGNED_MALLOC)
            return _aligned_malloc(bytes, alignment);
#else
#error "There's no aligned allocation support for your platform"
#endif
        },
        .pfnFree = [](void *, void *ptr) {
#if defined(HAS_C11_ALIGNED_ALLOC) || defined(HAS_POSIX_MEMALIGN)
            return free(ptr);
#elif defined(HAS_WIN32_ALIGNED_MALLOC)
            return _aligned_free(ptr);
#else
#error "There's no aligned allocation support for your platform"
#endif
        }
    };

    return result;
}

engine::rendering::VulkanState engine::rendering::VulkanState::with_tbbmalloc() noexcept
{
    VulkanState result;

    result.m_allocationCallbacks = VkAllocationCallbacks {
        .pfnAllocation = [](void *, size_t bytes, size_t alignment, VkSystemAllocationScope) { return scalable_aligned_malloc(bytes, alignment); },
        .pfnReallocation = [](void *, void *ptr, size_t bytes, size_t alignment, VkSystemAllocationScope) { return scalable_aligned_realloc(ptr, bytes, alignment); },
        .pfnFree = [](void *, void *ptr) { scalable_aligned_free(ptr); }
    };
    return result;
}

engine::rendering::VulkanState::~VulkanState()
{
    if (imgui_renderpass != VK_NULL_HANDLE)
        deviceTable.vkDestroyRenderPass(device, std::exchange(imgui_renderpass, VK_NULL_HANDLE), allocationCallbacks());
    if (sdl2_surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(instance, std::exchange(sdl2_surface, VK_NULL_HANDLE), allocationCallbacks());
    if (device != VK_NULL_HANDLE)
        deviceTable.vkDestroyDevice(std::exchange(device, VK_NULL_HANDLE), allocationCallbacks());
    if (instance != VK_NULL_HANDLE)
        vkDestroyInstance(std::exchange(instance, VK_NULL_HANDLE), allocationCallbacks());
}

std::error_category const &engine::rendering::vulkan_category() noexcept
{

    class VulkanErrorCode final : public std::error_category {
    public:
        char const *name() const noexcept override
        {
            return "VkResult";
        }

        std::string message(int code) const override
        {

            switch (code) {
            case VK_SUCCESS: return "Success";
            case VK_NOT_READY: return "NotReady";
            case VK_TIMEOUT: return "Timeout";
            case VK_EVENT_SET: return "EventSet";
            case VK_EVENT_RESET: return "EventReset";
            case VK_INCOMPLETE: return "Incomplete";
            case VK_ERROR_OUT_OF_HOST_MEMORY: return "ErrorOutOfHostMemory";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "ErrorOutOfDeviceMemory";
            case VK_ERROR_INITIALIZATION_FAILED: return "ErrorInitializationFailed";
            case VK_ERROR_DEVICE_LOST: return "ErrorDeviceLost";
            case VK_ERROR_MEMORY_MAP_FAILED: return "ErrorMemoryMapFailed";
            case VK_ERROR_LAYER_NOT_PRESENT: return "ErrorLayerNotPresent";
            case VK_ERROR_EXTENSION_NOT_PRESENT: return "ErrorExtensionNotPresent";
            case VK_ERROR_FEATURE_NOT_PRESENT: return "ErrorFeatureNotPresent";
            case VK_ERROR_INCOMPATIBLE_DRIVER: return "ErrorIncompatibleDriver";
            case VK_ERROR_TOO_MANY_OBJECTS: return "ErrorTooManyObjects";
            case VK_ERROR_FORMAT_NOT_SUPPORTED: return "ErrorFormatNotSupported";
            case VK_ERROR_FRAGMENTED_POOL: return "ErrorFragmentedPool";
            case VK_ERROR_UNKNOWN: return "ErrorUnknown";
            case VK_ERROR_OUT_OF_POOL_MEMORY: return "ErrorOutOfPoolMemory";
            case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "ErrorInvalidExternalHandle";
            case VK_ERROR_FRAGMENTATION: return "ErrorFragmentation";
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "ErrorInvalidOpaqueCaptureAddress";
            case VK_PIPELINE_COMPILE_REQUIRED: return "PipelineCompileRequired";
            case VK_ERROR_SURFACE_LOST_KHR: return "ErrorSurfaceLostKHR";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "ErrorNativeWindowInUseKHR";
            case VK_SUBOPTIMAL_KHR: return "SuboptimalKHR";
            case VK_ERROR_OUT_OF_DATE_KHR: return "ErrorOutOfDateKHR";
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "ErrorIncompatibleDisplayKHR";
            case VK_ERROR_VALIDATION_FAILED_EXT: return "ErrorValidationFailedEXT";
            case VK_ERROR_INVALID_SHADER_NV: return "ErrorInvalidShaderNV";
            case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "ErrorInvalidDrmFormatModifierPlaneLayoutEXT";
            case VK_ERROR_NOT_PERMITTED_KHR: return "ErrorNotPermittedKHR";
#if defined(VK_USE_PLATFORM_WIN32_KHR)
            case VK_ERROR_FULLSCREEN_EXCLUSIVE_MODE_LOST_EXT: return "ErrorFullScreenExclusiveModeLostEXT";
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
            case VK_THREAD_IDLE_KHR: return "ThreadIdleKHR";
            case VK_THREAD_DONE_KHR: return "ThreadDoneKHR";
            case VK_OPERATION_DEFERRED_KHR: return "OperationDeferredKHR";
            case VK_OPERATION_NOT_DEFERRED_KHR: return "OperationNotDeferredKHR";
            default: return fmt::format("unkown({:x})", static_cast<uint32_t>(code));
            }
        };
    };

    static auto const s_category = VulkanErrorCode {};
    return s_category;
}