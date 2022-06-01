#ifndef ENGINE_RENDERING_VULKAN_STATE_HPP
#define ENGINE_RENDERING_VULKAN_STATE_HPP

#ifndef ENGINE_WITH_VULKAN
#error "Engine is configured to not use vulkan but this file was included"
#endif

#include <volk.h>

#include <memory_resource>
#include <optional>

namespace engine::rendering {
    struct VulkanState {
    private:
        std::optional<VkAllocationCallbacks> m_allocationCallbacks = std::nullopt;

    public:
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

        ~VulkanState();
    };

    std::error_category const &vulkan_category() noexcept;

    inline std::error_code make_error_code(VkResult result) noexcept
    {
        return std::error_code(static_cast<int>(result), vulkan_category());
    }

}

static void CHECK_VK(VkResult result)
{
    if (result != VK_SUCCESS)
        throw std::system_error(engine::rendering::make_error_code(result));
}

#endif