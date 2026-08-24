#ifndef VELA_CORE_BACKEND_CONTEXT_IMPL_HPP
#define VELA_CORE_BACKEND_CONTEXT_IMPL_HPP

#include "volk.h"
#include "Vela/Core/IWindowBackend.hpp"
#include "Vela/Core/ContextPreferences.hpp"

#include "vk_mem_alloc.h"

#include <optional>

namespace vela::backend
{
    class ContextImpl
    {
    public:
        ContextImpl(core::IWindowBackend& windowBackend, const core::ContextPreferences& contextPreferences);
        ~ContextImpl();

        void setSurface(VkSurfaceKHR surface);
        void setNativeWindow(void* nativeWindow);

        void waitIdle() const;

        VkDevice getDevice() const;
        VkInstance getInstance() const;
        VkCommandPool getGraphicsCommandPool() const;
        VmaAllocator getAllocator() const;
        VkQueue getGraphicsQueue() const;
        VkSwapchainKHR getSwapchain() const;

        const std::vector<VkImage>& getSwapchainImages() const;
        const std::vector<VkImageView>& getSwapchainImageViews() const;
        VkImage getDepthImage() const;
        VkImageView getDepthImageView() const;
        
        VkExtent2D getSwapchainExtent() const;
        VkFormat getSwapchainFormat() const;
        VkFormat getDepthFormat() const;

        bool isSwapchainStale() const;

        void pickPhysicalDevice();
        void createDevice();
        void createSwapchain();
        void createSwapchainImageViews();
        void createDepthImage();
        void createCommandPool();
        void createAllocator();
        void recreateSwapchain();

    private:
        struct QueueFamilyIndices
        {
            std::optional<uint32_t> graphics;
            std::optional<uint32_t> compute;
            std::optional<uint32_t> transfer;
            std::optional<uint32_t> present;

            bool complete() const
            {
                return graphics.has_value() && compute.has_value() &&
                    transfer.has_value() && present.has_value();
            }
        };

        void createInstance(core::IWindowBackend& windowBackend);

        VkExtent2D framebufferExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

        bool checkInstanceExtensions(const std::vector<const char*>& extensions);
        bool checkValidationLayers(const std::vector<const char*>& requiredLayers);

        //TODO expose it to public API later
        static constexpr uint32_t m_vulkanApiVersion{VK_API_VERSION_1_3};
        static constexpr VkFormat m_depthFormat{VK_FORMAT_D32_SFLOAT};

        core::ContextPreferences m_contextPreferences;

        VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
        VkDevice m_device{VK_NULL_HANDLE};
        VkInstance m_instance{VK_NULL_HANDLE};
        VkSurfaceKHR m_surface{VK_NULL_HANDLE};
        void* m_nativeWindow{nullptr};

        QueueFamilyIndices m_queueFamilyIndices{};

        VkQueue m_graphicsQueue{VK_NULL_HANDLE};
        VkQueue m_transferQueue{VK_NULL_HANDLE};
        VkQueue m_computeQueue{VK_NULL_HANDLE};

        VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};
        VkFormat m_swapchainFormat{VK_FORMAT_UNDEFINED};   
        VkExtent2D m_swapchainExtent{};

        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;

        VkImage m_depthImage{VK_NULL_HANDLE};
        VmaAllocation m_depthImageAllocation{VK_NULL_HANDLE};
        VkImageView m_depthImageView{VK_NULL_HANDLE};

        VkCommandPool m_commandPool{VK_NULL_HANDLE};

        core::IWindowBackend& m_windowBackend;

        VmaAllocator m_allocator{VK_NULL_HANDLE};
    };

} // namespace vela::backend

#endif //VELA_CORE_BACKEND_CONTEXT_IMPL_HPP