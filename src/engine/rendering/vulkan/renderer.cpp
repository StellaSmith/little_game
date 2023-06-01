#include <engine/Game.hpp>
#include <engine/rendering/vulkan/Renderer.hpp>

#include <SDL_error.h>
#include <SDL_video.h>
#include <SDL_vulkan.h>
#include <fmt/ranges.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include <memory>
#include <spdlog/spdlog.h>

#include <volk.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

using namespace std::literals;

SDL_Window *engine::rendering::vulkan::Renderer::create_window(const char *title, int x, int y, int w, int h, uint32_t flags)
{
    auto const window = SDL_CreateWindow(title, x, y, w, h, flags | SDL_WINDOW_VULKAN);
    if (window == nullptr) {
        spdlog::error("failed to created SDL window: {}", SDL_GetError());
        throw std::runtime_error(SDL_GetError());
    }

    return window;
}

void engine::rendering::vulkan::Renderer::setup()
{
    auto const pfn_vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
    if (pfn_vkGetInstanceProcAddr == nullptr) {
        spdlog::error("SDL_Vulkan_GetVkGetInstanceProcAddr() failed: {}", SDL_GetError());
        throw std::runtime_error(SDL_GetError());
    }
    volkInitializeCustom(pfn_vkGetInstanceProcAddr);

    instance = [&]() {
        std::vector<char const *> instance_extensions;
        std::vector<char const *> instance_layers;

        {
            std::vector<char const *> required_extensions;
            std::vector<char const *> desired_extensions;

            unsigned count = 0;
            SDL_Vulkan_GetInstanceExtensions(game().window(), &count, nullptr);
            required_extensions.resize(count);
            SDL_Vulkan_GetInstanceExtensions(game().window(), &count, &required_extensions[0]);
#ifndef NDEBUG
            desired_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
            uint32_t extensionPropertiesCount;
            CHECK_VK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertiesCount, nullptr));
            auto extensionProperties = std::make_unique_for_overwrite<VkExtensionProperties[]>(extensionPropertiesCount);
            CHECK_VK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertiesCount, extensionProperties.get()));

            spdlog::info("available instance extensions ({}):", extensionPropertiesCount);
            for (std::size_t i = 0; i < extensionPropertiesCount; ++i) {
                auto const major = VK_VERSION_MAJOR(extensionProperties[i].specVersion);
                auto const minor = VK_VERSION_MINOR(extensionProperties[i].specVersion);
                auto const patch = VK_VERSION_PATCH(extensionProperties[i].specVersion);
                spdlog::info("\t{}: {}.{}.{}", extensionProperties[i].extensionName, major, minor, patch);
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

            spdlog::info("available instance layers ({}):", layerPropertiesCount);
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

        spdlog::info("selected instance extensions ({}):", instance_extensions.size());
        for (auto const extension : instance_extensions)
            spdlog::info("\t{}", extension);

        spdlog::info("selected instance layers ({}):", instance_layers.size());
        for (auto const layer : instance_layers)
            spdlog::info("\t{}", layer);

        VkApplicationInfo const application_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "vgame",
            .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
            .pEngineName = "vengine",
            .engineVersion = VK_MAKE_VERSION(0, 1, 0),
            .apiVersion = VK_API_VERSION_1_0,
        };

        VkInstanceCreateInfo instance_create_info {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &application_info,
            .enabledLayerCount = static_cast<uint32_t>(instance_layers.size()),
            .ppEnabledLayerNames = instance_layers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()),
            .ppEnabledExtensionNames = instance_extensions.data(),
        };

        VkInstance instance;
        CHECK_VK(vkCreateInstance(&instance_create_info, allocation_callbacks(), &instance));

        return instance;
    }();

    volkLoadInstanceOnly(instance);

    // vulkan handles are always 64bit,
    // so it's not enough to reinterpret_cast a handle to a pointer
    if (!ImGui_ImplVulkan_LoadFunctions(
            +[](char const *function_name, void *udata) {
                auto const instance = *reinterpret_cast<VkInstance const *>(udata);
                return vkGetInstanceProcAddr(instance, function_name);
            },
            const_cast<void *>(static_cast<void const *>(&instance)))) {
        char const *msg = "failed to dynamically load Vulkan functions for ImGui";
        spdlog::critical("{}", msg);
        throw std::runtime_error(msg);
    }

    if (!SDL_Vulkan_CreateSurface(game().window(), instance, &sdl2_surface)) {
        spdlog::error("failed to create Vulkan surface for SDL window: {}", SDL_GetError());
        throw std::runtime_error(SDL_GetError());
    }

    physical_device = [&] {
        uint32_t physical_deviceCount {};
        CHECK_VK(vkEnumeratePhysicalDevices(instance, &physical_deviceCount, nullptr));

        if (physical_deviceCount == 0) {
            char const *msg = "no physical vulkan devices available";
            spdlog::error("{}", msg);
            throw std::runtime_error(msg);
        }

        auto physical_devices = std::make_unique_for_overwrite<VkPhysicalDevice[]>(physical_deviceCount);
        CHECK_VK(vkEnumeratePhysicalDevices(instance, &physical_deviceCount, physical_devices.get()));

        spdlog::info("physical vulkan devices ({}):", physical_deviceCount);
        for (uint32_t i = 0; i < physical_deviceCount; ++i) {

            VkPhysicalDeviceProperties physical_deviceProperties;
            vkGetPhysicalDeviceProperties(physical_devices[i], &physical_deviceProperties);
            VkPhysicalDeviceFeatures physical_deviceFeatures;
            vkGetPhysicalDeviceFeatures(physical_devices[i], &physical_deviceFeatures);

            auto const device_type = [&] {
                switch (physical_deviceProperties.deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    return "CPU"sv;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    return "Discrete GPU"sv;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    return "Integrated GPU"sv;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    return "Virtual GPU"sv;
                default:
                    return "Unknown"sv;
                }
            }();
            spdlog::info("\t{} ID={}\tName={} ({})",
                i, physical_deviceProperties.deviceID,
                physical_deviceProperties.deviceName, device_type);
        }

        // TODO: properly select which device to use
        auto const &selected_device = physical_devices[0];

        VkPhysicalDeviceProperties physical_device_properties;
        vkGetPhysicalDeviceProperties(selected_device, &physical_device_properties);
        spdlog::info("selected device ID={}, Name={}", physical_device_properties.deviceID, physical_device_properties.deviceName);

        return physical_devices[0];
    }();

    queue_indices = [&] {
        uint32_t queueFamilyPropertiesCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyPropertiesCount, nullptr);
        auto const queueFamilyProperties = std::make_unique_for_overwrite<VkQueueFamilyProperties[]>(queueFamilyPropertiesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyPropertiesCount, queueFamilyProperties.get());

        auto result = decltype(queue_indices) {};

        spdlog::info("vulkan queue families ({}):", queueFamilyPropertiesCount);
        for (uint32_t i = 0; i < queueFamilyPropertiesCount; ++i) {
            auto const &queueFamilyProperty = queueFamilyProperties[i];

            std::vector<char const *> bits_names;
            if (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                bits_names.push_back("VK_QUEUE_GRAPHICS_BIT");
            if (queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT)
                bits_names.push_back("VK_QUEUE_COMPUTE_BIT");
            if (queueFamilyProperty.queueFlags & VK_QUEUE_TRANSFER_BIT)
                bits_names.push_back("VK_QUEUE_TRANSFER_BIT");
            if (queueFamilyProperty.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                bits_names.push_back("VK_QUEUE_SPARSE_BINDING_BIT");
            if (queueFamilyProperty.queueFlags & VK_QUEUE_PROTECTED_BIT)
                bits_names.push_back("VK_QUEUE_PROTECTED_BIT");
            if (queueFamilyProperty.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)
                bits_names.push_back("VK_QUEUE_OPTICAL_FLOW_BIT_NV");

            VkBool32 present_support = false;
            CHECK_VK(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, sdl2_surface, &present_support));

            spdlog::info("\t{}: Flags={}\tPresentSupport={}", i, bits_names, static_cast<bool>(present_support));

            if (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT && result.graphics == UINT32_MAX)
                result.graphics = i;

            if (present_support && result.present == UINT32_MAX)
                result.present = i;

            if (queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT && result.compute == UINT32_MAX)
                result.compute = i;
        }
        spdlog::info("selected queue index {} for graphics", result.graphics);
        spdlog::info("selected queue index {} for present", result.present);
        spdlog::info("selected queue index {} for compute", result.compute);

        return result;
    }();

    device = [&] {
        spdlog::info("creating vulkan logical device");

        auto const unique_queue_indices = [&]() {
            auto unique_queue_indices = std::unordered_map<std::uint32_t, std::size_t>(std::size(queue_indices_array));
            for (auto const queue_index : queue_indices_array) {
                if (auto it = unique_queue_indices.find(queue_index); it != unique_queue_indices.end())
                    unique_queue_indices.emplace_hint(it, std::make_pair(queue_index, std::size_t { 1 }));
                else
                    ++it->second;
            }
            return unique_queue_indices;
        }();

        // since we will set all priorities to 1.0f, and we can have at most `std::size(queue_indices_array)` equal queue indices,
        // we allocate an array big enough to hold as many 1.0fs
        auto const priorities = [&]() {
            auto priorities = std::make_unique_for_overwrite<float[]>(std::size(queue_indices_array));
            std::fill_n(&priorities[0], std::size(queue_indices_array), 1.0f);
            return std::unique_ptr<float const[]>(std::move(priorities));
        }();

        auto const device_queue_create_infos = [&]() {
            auto device_queue_create_infos = std::make_unique_for_overwrite<VkDeviceQueueCreateInfo[]>(unique_queue_indices.size());
            auto i = std::size_t {};

            for (auto const &[queue_index, count] : unique_queue_indices) {
                device_queue_create_infos[i] = VkDeviceQueueCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .queueFamilyIndex = queue_index,
                    .queueCount = static_cast<uint32_t>(count),
                    .pQueuePriorities = &priorities[0],
                };
            }
            return std::unique_ptr<VkDeviceQueueCreateInfo const[]>(std::move(device_queue_create_infos));
        }();

        auto const device_features = VkPhysicalDeviceFeatures {
            // TODO: fill up with what we need
        };

        auto const device_create_info = VkDeviceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = static_cast<uint32_t>(unique_queue_indices.size()),
            .pQueueCreateInfos = &device_queue_create_infos[0],
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = 0,
            .ppEnabledExtensionNames = nullptr,
            .pEnabledFeatures = &device_features,
        };

        VkDevice device;
        CHECK_VK(vkCreateDevice(physical_device, &device_create_info, allocation_callbacks(), &device));
        return device;
    }();

    volkLoadDeviceTable(&device_table, device);

    queues = [&] {
        auto const unique_queue_indices = [&]() {
            auto unique_queue_indices = std::unordered_map<std::uint32_t, std::size_t>(std::size(queue_indices_array));
            for (auto const queue_index : queue_indices_array) {
                if (auto it = unique_queue_indices.find(queue_index); it != unique_queue_indices.end())
                    unique_queue_indices.emplace_hint(it, std::make_pair(queue_index, std::size_t { 1 }));
                else
                    ++it->second;
            }
            return unique_queue_indices;
        }();

        auto result = decltype(queues) {};
        if (queue_indices.compute == queue_indices.graphics) {
            device_table.vkGetDeviceQueue(device, queue_indices.graphics, 0, &result.graphics);
            device_table.vkGetDeviceQueue(device, queue_indices.graphics, 1, &result.compute);
        } else {
            device_table.vkGetDeviceQueue(device, queue_indices.graphics, 0, &result.graphics);
            device_table.vkGetDeviceQueue(device, queue_indices.compute, 0, &result.compute);
        }
        return result;
    }();
}

void engine::rendering::vulkan::Renderer::imgui_setup()
{
    ImGui_ImplVulkan_InitInfo vulkan_init_info {
        .Instance = instance,
        .PhysicalDevice = physical_device,
        .Device = device,
        .QueueFamily = queue_indices.graphics,
        .Queue = queues.graphics,
        .PipelineCache = VK_NULL_HANDLE,
        .DescriptorPool = VK_NULL_HANDLE,
        .Subpass = 0,
        .MinImageCount = 0,
        .ImageCount = 0,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .Allocator = allocation_callbacks(),
        .CheckVkResultFn = &CHECK_VK,
    };

    ImGui_ImplVulkan_Init(&vulkan_init_info, VK_NULL_HANDLE);
    ImGui_ImplSDL2_InitForVulkan(game().window());
}

void engine::rendering::vulkan::Renderer::imgui_new_frame(SDL_Window *window)
{
    ImGui_ImplSDL2_NewFrame(window);
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
}

void engine::rendering::vulkan::Renderer::render(float)
{

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), VK_NULL_HANDLE);
}

void engine::rendering::vulkan::Renderer::update()
{
}

engine::rendering::vulkan::Renderer::~Renderer()
{
    ImGui_ImplVulkan_Shutdown();
    if (imgui_renderpass != VK_NULL_HANDLE)
        device_table.vkDestroyRenderPass(device, std::exchange(imgui_renderpass, VK_NULL_HANDLE), allocation_callbacks());
    if (sdl2_surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(instance, std::exchange(sdl2_surface, VK_NULL_HANDLE), allocation_callbacks());
    if (device != VK_NULL_HANDLE)
        device_table.vkDestroyDevice(std::exchange(device, VK_NULL_HANDLE), allocation_callbacks());
    if (instance != VK_NULL_HANDLE)
        vkDestroyInstance(std::exchange(instance, VK_NULL_HANDLE), allocation_callbacks());
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
std::error_category const &engine::rendering::vulkan::category() noexcept
{
    return s_category;
}