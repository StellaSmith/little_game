
#include <engine/Game.hpp>
#include <engine/rendering/vulkan/Renderer.hpp>

#include <fmt/core.h>

#include <utility>

void engine::rendering::vulkan::Renderer::setup()
{
    volkInitializeCustom(reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr()));

    vulkan.instance = [&]() {
        std::vector<char const *> instance_extensions;
        std::vector<char const *> instance_layers;

        {
            std::vector<char const *> required_extensions;
            std::vector<char const *> desired_extensions;

            unsigned count = 0;
            SDL_Vulkan_GetInstanceExtensions(window.get(), &count, nullptr);
            required_extensions.resize(count);
            SDL_Vulkan_GetInstanceExtensions(window.get(), &count, &required_extensions[0]);

            desired_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            uint32_t extensionPropertiesCount;
            CHECK_VK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertiesCount, nullptr));
            auto extensionProperties = std::make_unique_for_overwrite<VkExtensionProperties[]>(extensionPropertiesCount);
            CHECK_VK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertiesCount, extensionProperties.get()));
            if (g_verbose) {
                spdlog::info("Avaible extensions ({}):", extensionPropertiesCount);
                for (std::size_t i = 0; i < extensionPropertiesCount; ++i) {
                    auto const major = VK_VERSION_MAJOR(extensionProperties[i].specVersion);
                    auto const minor = VK_VERSION_MINOR(extensionProperties[i].specVersion);
                    auto const patch = VK_VERSION_PATCH(extensionProperties[i].specVersion);
                    spdlog::info("\t{}: {}.{}.{}", extensionProperties[i].extensionName, major, minor, patch);
                }
            }

            instance_extensions.insert(instance_extensions.end(), required_extensions.begin(), required_extensions.end());

            for (auto const desired : desired_extensions) {
                for (uint32_t i = 0; i < extensionPropertiesCount; ++i) {
                    if (std::strcmp(extensionProperties[i].extensionName, desired) == 0) {
                        instance_extensions.push_back(desired);
                        break;
                    }
                }
            }
        }

        {
            std::vector<char const *> required_layers;
            std::vector<char const *> desired_layers;
#ifndef NDEBUG
            desired_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

            uint32_t layerPropertiesCount;
            CHECK_VK(vkEnumerateInstanceLayerProperties(&layerPropertiesCount, nullptr));
            auto layerProperties = std::make_unique_for_overwrite<VkLayerProperties[]>(layerPropertiesCount);
            CHECK_VK(vkEnumerateInstanceLayerProperties(&layerPropertiesCount, layerProperties.get()));
            if (g_verbose) {
                spdlog::info("Avaible extensions ({}):", layerPropertiesCount);
                for (std::size_t i = 0; i < layerPropertiesCount; ++i) {
                    auto const major = VK_VERSION_MAJOR(layerProperties[i].specVersion);
                    auto const minor = VK_VERSION_MINOR(layerProperties[i].specVersion);
                    auto const patch = VK_VERSION_PATCH(layerProperties[i].specVersion);

                    auto const impl_major = VK_VERSION_MAJOR(layerProperties[i].implementationVersion);
                    auto const impl_minor = VK_VERSION_MINOR(layerProperties[i].implementationVersion);
                    auto const impl_patch = VK_VERSION_PATCH(layerProperties[i].implementationVersion);

                    spdlog::info("\t{}: {}.{}.{} ({}.{}.{})",
                        layerProperties[i].layerName,
                        major, minor, patch,
                        impl_major, impl_minor, impl_patch);
                    spdlog::info("\t\t{}", layerProperties[i].description);
                }
            }

            instance_layers.insert(instance_layers.end(), required_layers.begin(), required_layers.end());

            for (auto const desired : desired_layers) {
                for (uint32_t i = 0; i < layerPropertiesCount; ++i) {
                    if (std::strcmp(layerProperties[i].layerName, desired) == 0) {
                        instance_layers.push_back(desired);
                        break;
                    }
                }
            }
        }

        VkApplicationInfo const applicationInfo {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "vgame",
            .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
            .pEngineName = "vengine",
            .engineVersion = VK_MAKE_VERSION(0, 1, 0),
            .apiVersion = VK_API_VERSION_1_0,
        };

        VkInstanceCreateInfo instanceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = static_cast<uint32_t>(instance_layers.size()),
            .ppEnabledLayerNames = instance_layers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()),
            .ppEnabledExtensionNames = instance_extensions.data(),
        };

        VkInstance instance;
        CHECK_VK(vkCreateInstance(&instanceCreateInfo, vulkan.allocationCallbacks(), &instance));

        return instance;
    }();

    volkLoadInstanceOnly(vulkan.instance);

    // vulkan handles are always 64bit,
    // so it's not enough to reinterpret_cast a handle to a pointer
    if (!ImGui_ImplVulkan_LoadFunctions(
            +[](char const *function_name, void *udata) {
                auto const instance = *reinterpret_cast<VkInstance const *>(udata);
                return vkGetInstanceProcAddr(instance, function_name);
            },
            const_cast<void *>(static_cast<void const *>(&vulkan.instance)))) {
        spdlog::critical("Cannot load Vulkan functions");
        std::exit(-1);
    };

    if (!SDL_Vulkan_CreateSurface(window.get(), vulkan.instance, &vulkan.sdl2_surface))
        throw SDL::Error::current();

    vulkan.physicalDevice = [&] {
        uint32_t physicalDeviceCount {};
        CHECK_VK(vkEnumeratePhysicalDevices(vulkan.instance, &physicalDeviceCount, nullptr));
        auto physicalDevices = std::make_unique_for_overwrite<VkPhysicalDevice[]>(physicalDeviceCount);
        CHECK_VK(vkEnumeratePhysicalDevices(vulkan.instance, &physicalDeviceCount, physicalDevices.get()));

        if (g_verbose)
            spdlog::info("Vulkan devices ({}):", physicalDeviceCount);
        for (uint32_t i = 0; i < physicalDeviceCount; ++i) {

            VkPhysicalDeviceProperties physicalDeviceProperties;
            vkGetPhysicalDeviceProperties(physicalDevices[i], &physicalDeviceProperties);
            VkPhysicalDeviceFeatures physicalDeviceFeatures;
            vkGetPhysicalDeviceFeatures(physicalDevices[i], &physicalDeviceFeatures);

            if (g_verbose) {
                auto const deviceType = [&] {
                    switch (physicalDeviceProperties.deviceType) {
                    case VK_PHYSICAL_DEVICE_TYPE_CPU:
                        return "CPU"sv;
                    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                        return "Discrete GPU"sv;
                    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                        return "Integrated GPU"sv;
                    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                        return "Virtual GPU"sv;
                    default:
                        return "Unkown"sv;
                    }
                }();
                spdlog::info("\t{} ID: {}\tName: {} ({})",
                    i, physicalDeviceProperties.deviceID,
                    physicalDeviceProperties.deviceName, deviceType);
            }
        }

        return physicalDevices[0];
    }();

    vulkan.queueFamiliyIndices = [&] {
        uint32_t queueFamilyPropertiesCount;
        vkGetPhysicalDeviceQueueFamilyProperties(vulkan.physicalDevice, &queueFamilyPropertiesCount, nullptr);
        auto const queueFamilyProperties = std::make_unique_for_overwrite<VkQueueFamilyProperties[]>(queueFamilyPropertiesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(vulkan.physicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties.get());

        auto result = decltype(vulkan.queueFamiliyIndices) {};

        for (uint32_t i = 0; i < queueFamilyPropertiesCount; ++i) {
            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                result.graphics = i;
            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                result.compute = i;
        }

        return result;
    }();

    vulkan.device = [&] {
        if (vulkan.queueFamiliyIndices.compute == vulkan.queueFamiliyIndices.graphics) {
            float const priorities[] = { 1.0f, 1.0f };
            VkDeviceQueueCreateInfo const deviceQueueCreateInfos {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = vulkan.queueFamiliyIndices.graphics,
                .queueCount = 2,
                .pQueuePriorities = &priorities[0],
            };

            VkDeviceCreateInfo const deviceCreateInfo {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount = 1,
                .pQueueCreateInfos = &deviceQueueCreateInfos,
            };
            VkDevice device;
            CHECK_VK(vkCreateDevice(vulkan.physicalDevice, &deviceCreateInfo, vulkan.allocationCallbacks(), &device));
            return device;
        } else {
            float const priorities = 1.0f;
            VkDeviceQueueCreateInfo const deviceQueueCreateInfos[2] {
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueFamilyIndex = vulkan.queueFamiliyIndices.graphics,
                    .queueCount = 1,
                    .pQueuePriorities = &priorities,
                },
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueFamilyIndex = vulkan.queueFamiliyIndices.compute,
                    .queueCount = 1,
                    .pQueuePriorities = &priorities,
                }
            };

            VkDeviceCreateInfo const deviceCreateInfo {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount = std::size(deviceQueueCreateInfos),
                .pQueueCreateInfos = &deviceQueueCreateInfos[0],
            };
            VkDevice device;
            CHECK_VK(vkCreateDevice(vulkan.physicalDevice, &deviceCreateInfo, vulkan.allocationCallbacks(), &device));
            return device;
        }
    }();

    volkLoadDeviceTable(&vulkan.deviceTable, vulkan.device);

    vulkan.queues = [&] {
        auto result = decltype(vulkan.queues) {};
        if (vulkan.queueFamiliyIndices.compute == vulkan.queueFamiliyIndices.graphics) {
            vulkan.deviceTable.vkGetDeviceQueue(vulkan.device, vulkan.queueFamiliyIndices.graphics, 0, &result.graphics);
            vulkan.deviceTable.vkGetDeviceQueue(vulkan.device, vulkan.queueFamiliyIndices.graphics, 1, &result.compute);
        } else {
            vulkan.deviceTable.vkGetDeviceQueue(vulkan.device, vulkan.queueFamiliyIndices.graphics, 0, &result.graphics);
            vulkan.deviceTable.vkGetDeviceQueue(vulkan.device, vulkan.queueFamiliyIndices.compute, 0, &result.compute);
        }
        return result;
    }();
}

#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

void engine::rendering::vulkan::Renderer::setup_imgui()
{
    ImGui_ImplVulkan_InitInfo vulkanInitInfo {
        .Instance = vulkan.instance,
        .PhysicalDevice = vulkan.physicalDevice,
        .Device = vulkan.device,
        .QueueFamily = vulkan.queueFamiliyIndices.graphics,
        .Queue = vulkan.queues.graphics,
        .CheckVkResultFn = &CHECK_VK,
    };

    ImGui_ImplVulkan_Init(&vulkanInitInfo, VK_NULL_HANDLE);
    ImGui_ImplSDL2_InitForVulkan(window);
}

void engine::rendering::vulkan::Renderer::update()
{
    ImGui_ImplVulkan_NewFrame();
}

engine::rendering::vulkan::Renderer::~Renderer()
{
    ImGui_ImplVulkan_Shutdown();
    if (imgui_renderpass != VK_NULL_HANDLE)
        deviceTable.vkDestroyRenderPass(device, std::exchange(imgui_renderpass, VK_NULL_HANDLE), allocationCallbacks());
    if (sdl2_surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(instance, std::exchange(sdl2_surface, VK_NULL_HANDLE), allocationCallbacks());
    if (device != VK_NULL_HANDLE)
        deviceTable.vkDestroyDevice(std::exchange(device, VK_NULL_HANDLE), allocationCallbacks());
    if (instance != VK_NULL_HANDLE)
        vkDestroyInstance(std::exchange(instance, VK_NULL_HANDLE), allocationCallbacks());
}

namespace {
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
}

static auto const s_category = VulkanErrorCode {};
std::error_category const &engine::rendering::vulkan_category() noexcept
{
    return s_category;
}