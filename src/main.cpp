#include <SDL.hpp>
#include <engine/Config.hpp>
#include <engine/Game.hpp>
#include <utils/error.hpp>
#ifdef ENGINE_WITH_OPENGL
#include <glDebug.h>
#endif

#ifdef ENGINE_WITH_OPENGL
#include <imgui_impl_opengl3.h>
#endif
#ifdef ENGINE_WITH_VULKAN
#include <volk.h>

#include <SDL_vulkan.h>
#include <imgui_impl_vulkan.h>
#endif

#include <fmt/format.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <oneapi/tbb/scalable_allocator.h>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <system_error>

using namespace std::literals;

bool g_verbose = false;
static void fix_current_directory(char const *argv0);

struct VulkanState {
private:
    std::optional<VkAllocationCallbacks> m_allocationCallbacks = std::nullopt;

public:
    static VulkanState with_malloc() noexcept
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

    static VulkanState with_tbbmalloc()
    {
        VulkanState result;

        result.m_allocationCallbacks = VkAllocationCallbacks {
            .pfnAllocation = [](void *, size_t bytes, size_t alignment, VkSystemAllocationScope scope) { return scalable_aligned_malloc(bytes, alignment); },
            .pfnReallocation = [](void *, void *ptr, size_t bytes, size_t alignment, VkSystemAllocationScope scope) { return scalable_aligned_realloc(ptr, bytes, alignment); },
            .pfnFree = [](void *, void *ptr) { scalable_aligned_free(ptr); }
        };
        return result;
    }

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    VolkDeviceTable deviceTable {};

    struct {
        uint32_t graphics = UINT32_MAX;
        uint32_t compute = UINT32_MAX;
    } queueFamiliyIndices;

    struct {
        VkQueue graphics = VK_NULL_HANDLE;
        VkQueue compute = VK_NULL_HANDLE;
    } queues;

    VkSurfaceKHR sdl2_surface = VK_NULL_HANDLE;
    VkRenderPass imgui_renderpass = VK_NULL_HANDLE;

    VkAllocationCallbacks *allocationCallbacks() noexcept
    {
        if (m_allocationCallbacks.has_value())
            return &m_allocationCallbacks.value();
        else
            return nullptr;
    }

    VkAllocationCallbacks const *allocationCallbacks() const noexcept
    {
        if (m_allocationCallbacks.has_value())
            return &m_allocationCallbacks.value();
        else
            return nullptr;
    }

    ~VulkanState()
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
};

static std::error_category const &vulkan_category() noexcept
{

    class VulkanErrorCode final : public std::error_category {
    public:
        char const *name() const noexcept override
        {
            return "VkError";
        }
        std::string message(int code) const override
        {
            auto const result = static_cast<VkResult>(code);
            switch (result) {
            case VkResult::VK_SUCCESS:
                return "Command successfully completed";
            case VkResult::VK_NOT_READY:
                return "A fence or query has not yet completed";
            case VkResult::VK_TIMEOUT:
                return "A wait operation has not completed in the specified time";
            case VkResult::VK_EVENT_SET:
                return "An event is signaled";
            case VkResult::VK_EVENT_RESET:
                return "An event is unsignaled";
            case VkResult::VK_INCOMPLETE:
                return "A return array was too small for the result";
            default: return "An unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred";
            }
        }
    };

    static auto const s_category = VulkanErrorCode {};
    return s_category;
}

static std::error_code make_error_code(VkResult result) noexcept
{
    return std::error_code(static_cast<int>(result), vulkan_category());
}

static void CHECK_VK(VkResult result)
{
    if (result != VK_SUCCESS)
        throw std::system_error(make_error_code(result));
}

int main(int argc, char **argv)
{
#ifndef NDEBUG
    if (auto logger = spdlog::default_logger(); logger->level() < spdlog::level::debug)
        logger->set_level(spdlog::level::debug);
#endif

#ifdef SDL_MAIN_HANDLED
    SDL_SetMainReady();
#endif
#ifndef NDEBUG
    try {
#endif
        for (int i = 0; i < argc; ++i) {
            if (argv[i] == "-h"sv || argv[i] == "--help"sv) {
                fmt::print(
                    "Usage:\n"
                    "\t{} [options]\n"
                    "Options:\n"
                    "\t-h\t--help\t\tDisplay this message and exit.\n"
                    "\t-v\t--verbose\tDisplay more verbose messages.\n"
                    "\t--sdl-video-drivers\tEnumerate the aviable video drivers.\n"
                    "\t--sdl-audio-drivers\tEnumerate the avaible audio drivers.\n",
                    argv[0]);
                return 0;
            } else if (argv[i] == "--sdl-video-drivers"sv) {
                int drivers = SDL_GetNumVideoDrivers();
                spdlog::info("Available video drivers ({}):\n", drivers);
                for (int i = 0; i < drivers; ++i)
                    spdlog::info("\t{}) {}\n", i + 1, SDL_GetVideoDriver(i));
            } else if (argv[i] == "--sdl-audio-drivers"sv) {
                auto const drivers = SDL::Audio::drivers();
                spdlog::info("Available audio drivers ({}):\n", drivers.size());
                for (char const *driver : drivers)
                    spdlog::info("\t{}) {}\n", i + 1, driver);

            } else if (argv[i] == "-v"sv || argv[i] == "--verbose"sv) {
                g_verbose = true;
                spdlog::info("Verbose output enabled");
            }
        }
        fix_current_directory(argv[0]);

        auto sdl = SDL { 0 };
        auto &config = engine::load_engine_config();
        sdl.init_video(config.sdl.video_driver.c_str());
        sdl.init_audio(config.sdl.audio_driver.c_str());

        constexpr int width = 640, height = 480;

        auto window = sdl
                          .video()
                          .window_builder()
                          .set_title("My little game")
                          .set_dimensions(width, height)
                          .resizable()
                          .shown()
#ifdef ENGINE_WITH_OPENGL
                          .opengl()
                          .set_gl_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3)
                          .set_gl_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 3)
                          .set_gl_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE)
                          .set_gl_attribute(SDL_GL_RED_SIZE, config.opengl.red_bits)
                          .set_gl_attribute(SDL_GL_GREEN_SIZE, config.opengl.green_bits)
                          .set_gl_attribute(SDL_GL_BLUE_SIZE, config.opengl.blue_bits)
                          .set_gl_attribute(SDL_GL_ALPHA_SIZE, config.opengl.alpha_bits)
                          .set_gl_attribute(SDL_GL_DEPTH_SIZE, config.opengl.depth_bits)
                          .set_gl_attribute(SDL_GL_STENCIL_SIZE, config.opengl.stencil_bits)
#ifndef NDEBUG
                          .set_gl_attribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG)
#endif
#endif
#ifdef ENGINE_WITH_VULKAN
                          .vulkan()
#endif
                          .create();

#ifndef NDEBUG
        if (SDL_SetRelativeMouseMode(SDL_TRUE))
            utils::show_error("Can't set mouse to relative mode!"sv);
#endif

#if ENGINE_WITH_OPENGL
        auto gl_context = window.opengl_context();
        gl_context.make_current();

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&SDL_GL_GetProcAddress)))
            utils::show_error("GLAD Error."sv, "Failed to initialize the OpenGL context."sv);

        if (g_verbose) {
            int major, minor, r, g, b, a, d, s;
            SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
            SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
            SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &r);
            SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
            SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &b);
            SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);
            SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &d);
            SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &s);

            char unsigned const *version = glGetString(GL_VERSION);
            char unsigned const *vendor = glGetString(GL_VENDOR);
            char unsigned const *renderer = glGetString(GL_RENDERER);
            char unsigned const *shading_version = glGetString(GL_SHADING_LANGUAGE_VERSION);

            spdlog::info("OpenGL profile: {}.{}", major, minor);
            spdlog::info("\tRGBA bits: {}, {}, {}, {}", r, g, b, a);
            spdlog::info("\tDepth bits: {}", d);
            spdlog::info("\tStencil bits: {}", s);
            spdlog::info("\tVersion: {}", version);
            spdlog::info("\tVendor: {}", vendor);
            spdlog::info("\tRendered: {}", renderer);
            spdlog::info("\tShading language version: {}", shading_version);
        }
#if defined(GL_KHR_debug)
        if (GLAD_GL_KHR_debug) {
            int flags;
            glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
            if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback(glDebugOutput, nullptr);
                glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
                if (g_verbose)
                    spdlog::info("\tDebug output enabled.\n");
            }
        }
#endif
        SDL_GL_SetSwapInterval(0);
#endif

#ifdef ENGINE_WITH_VULKAN
        auto vulkan = VulkanState::with_tbbmalloc();

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

#endif

        if (!IMGUI_CHECKVERSION())
            utils::show_error("ImGui version mismatch!\nYou may need to recompile the game."sv);

        ImGui::CreateContext();

#ifdef ENGINE_WITH_VULKAN
        ImGui_ImplVulkan_InitInfo vulkanInitInfo {
            .Instance = vulkan.instance,
            .PhysicalDevice = vulkan.physicalDevice,
            .Device = vulkan.device,
            .QueueFamily = vulkan.queueFamiliyIndices.graphics,
            .Queue = vulkan.queues.graphics,
            .CheckVkResultFn = &CHECK_VK,
        };

        ImGui_ImplVulkan_Init(&vulkanInitInfo, VK_NULL_HANDLE);
#endif

        ImGuiIO &imgui_io = ImGui::GetIO();
        imgui_io.IniFilename = "cfg/imgui.ini";
        imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        if (SDL_WasInit(SDL_INIT_GAMECONTROLLER))
            imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

        // TODO: Make color scheme of imgui configurable
        ImGui::StyleColorsDark();

#if ENGINE_WITH_OPENGL
        ImGui_ImplSDL2_InitForOpenGL(window.get(), gl_context.get()); // always returns true

        // See imgui/examples/imgui_impl_opengl3.cpp
        ImGui_ImplOpenGL3_Init("#version 330 core"); // always returns true
#endif

#if ENGINE_WITH_VULKAN
        // ImGui_ImplVulkan_Init();
        ImGui_ImplSDL2_InitForVulkan(window.get());
#endif

        if (!config.imgui.font_path.empty()) {
            if (!imgui_io.Fonts->AddFontFromFileTTF(config.imgui.font_path.data(), 14)) {
                spdlog::error("[ImGui] Can't load font \"{}\", using default font.", config.imgui.font_path);
                if (!imgui_io.Fonts->AddFontDefault())
                    utils::show_error("ImGui Error."sv, "Can't load default font."sv);
            }
        } else {
            if (!imgui_io.Fonts->AddFontDefault())
                utils::show_error("ImGui Error."sv, "Can't load default font!"sv);
        }

        engine::Game game;
        game.start();

        auto start = engine::Game::clock_type::now();

        SDL_Event event;
        while (game.running) {
            auto now = engine::Game::clock_type::now();
            auto delta = now - start;
            start = now;

            while (SDL_PollEvent(&event) && game.running) {
                // Poll and handle events (inputs, window resize, etc.)
                // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
                // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
                // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
                // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
                ImGui_ImplSDL2_ProcessEvent(&event);
                game.input(event);
            }

#if ENGINE_WITH_OPENGL
            ImGui_ImplOpenGL3_NewFrame();
#endif
#ifdef ENGINE_WITH_VULKAN
            ImGui_ImplVulkan_NewFrame();
#endif
            ImGui_ImplSDL2_NewFrame(window.get());
            ImGui::NewFrame();

            game.update(delta);
            if (!game.running) break;
            game.render();

            // Draw ImGui on top of the game stuff
            ImGui::Render();
#if ENGINE_WITH_OPENGL
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // swap the buffer (present to the window surface)
            SDL_GL_SwapWindow(window.get());
#endif
        }

        game.cleanup();

#if ENGINE_WITH_OPENGL
        ImGui_ImplOpenGL3_Shutdown();
#endif
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        return 0;
#ifndef NDEBUG
    } catch (sol::error const &e) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Lua panic!", e.what(), nullptr);
        spdlog::critical("Lua panic!: {}", e.what());
        throw;
    } catch (utils::application_error const &e) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, e.title().data(), e.body().data(), nullptr);
        spdlog::critical("{}: {}", e.title(), e.body());
        throw;
    } catch (std::exception const &e) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "EXCEPTION NOT HANDLED!!", e.what(), nullptr);
        spdlog::critical("std::exception raised: {}", e.what());
        throw;
    } catch (...) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "UNKOWN EXCEPTION NOT HANDLED!!!", "UNKOWN EXCEPTION NOT HANDLED!!!", nullptr);
        spdlog::critical("Unkown exception raised");
        throw;
    }
#endif
    return -1;
}

static void fix_current_directory(char const *argv0)
{
    auto const path = std::filesystem::path(argv0, std::filesystem::path::native_format).parent_path().parent_path();
    std::filesystem::current_path(path);

    if (g_verbose)
        spdlog::info("Working directory: {}", path.string());
}