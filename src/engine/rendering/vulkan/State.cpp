#include <engine/rendering/vulkan/State.hpp>

#include <fmt/core.h>

#include <utility>

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