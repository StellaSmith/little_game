#ifndef ENGINE_WITH_VULKAN
#error "Engine is configured to not use Vulkan but this file was included"
#endif

#ifndef ENGINE_RENDERING_VULKAN_RENDERER_HPP
#define ENGINE_RENDERING_VULKAN_RENDERER_HPP

#include <volk.h>

#include <engine/rendering/IRenderer.hpp>

#include <memory_resource>
#include <optional>

namespace engine::rendering::vulkan {
    class Renderer final : public virtual IRenderer {
    public:
        using IRenderer::IRenderer;

    private:
        std::optional<VkAllocationCallbacks> m_allocation_callbacks = std::nullopt;
        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;

        VolkDeviceTable device_table {};

        struct {
            uint32_t graphics = UINT32_MAX;
            uint32_t compute = UINT32_MAX;
        } queue_familiy_indices;

        struct {
            VkQueue graphics = VK_NULL_HANDLE;
            VkQueue compute = VK_NULL_HANDLE;
        } queues;

        VkSurfaceKHR sdl2_surface = VK_NULL_HANDLE;
        VkRenderPass imgui_renderpass = VK_NULL_HANDLE;

        VkAllocationCallbacks *allocation_callbacks() noexcept
        {
            if (m_allocation_callbacks.has_value())
                return &m_allocation_callbacks.value();
            else
                return nullptr;
        }

        VkAllocationCallbacks const *allocation_callbacks() const noexcept
        {
            if (m_allocation_callbacks.has_value())
                return &m_allocation_callbacks.value();
            else
                return nullptr;
        }

    public:
        SDL_Window *create_window(char const *title, int x, int y, int w, int h, uint32_t flags) override;

        void setup() override;
        void update() override;
        void render(float delta) override;

        void imgui_setup() override;
        void imgui_new_frame(SDL_Window *) override;

        ~Renderer() override;
    };

    std::error_category const &category() noexcept;

    inline std::error_code make_error_code(VkResult result) noexcept
    {
        return std::error_code(static_cast<int>(result), category());
    }

}

namespace {
    [[maybe_unused]] inline void CHECK_VK(VkResult result)
    {
        if (result != VK_SUCCESS)
            throw std::system_error(engine::rendering::vulkan::make_error_code(result));
    }
}

#endif