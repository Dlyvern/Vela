#ifndef VELA_CORE_BACKEND_CONTEXT_IMPL_HPP
#define VELA_CORE_BACKEND_CONTEXT_IMPL_HPP

#include "volk.h"
#include "Vela/Core/IWindowBackend.hpp"

#include "vk_mem_alloc.h"

namespace vela::backend
{
    class ContextImpl
    {
    public:
        ContextImpl(core::IWindowBackend& windowBackend);
        ~ContextImpl();

        void setSurface(VkSurfaceKHR surface);

        VkDevice getDevice() const;
        VkInstance getInstance() const;
        VkRenderPass getRenderPass() const;
        VkCommandPool getGraphicsCommandPool() const;
        VmaAllocator getAllocator() const;
        VkQueue getGraphicsQueue() const;
        VkSwapchainKHR getSwapchain() const;
        const std::vector<VkImage>& getSwapchainImages() const;
        const std::vector<VkImageView>& getSwapchainImageViews() const;
        VkExtent2D getSwapchainExtent() const;

        void pickPhysicalDevice();
        void createDevice();
        void createSwapchain();
        void createSwapchainImageViews();
        void createRenderPass();
        void createCommandPool();
        void createAllocator();
        void recreateSwapchain();

    private:
        void createInstance(core::IWindowBackend& windowBackend);

        VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
        VkDevice m_device{VK_NULL_HANDLE};
        VkInstance m_instance{VK_NULL_HANDLE};
        VkSurfaceKHR m_surface{VK_NULL_HANDLE};
        uint32_t m_graphicsFamily{0};
        VkQueue m_graphicsQueue{VK_NULL_HANDLE};

        VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};
        VkFormat m_swapchainFormat{VK_FORMAT_UNDEFINED};   
        VkExtent2D m_swapchainExtent{};
        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;

        VkRenderPass m_renderPass{VK_NULL_HANDLE};

        VkCommandPool m_commandPool{VK_NULL_HANDLE};

        core::IWindowBackend& m_windowBackend;

        VmaAllocator m_allocator{VK_NULL_HANDLE};
    };

} // namespace vela::backend

#endif //VELA_CORE_BACKEND_CONTEXT_IMPL_HPP