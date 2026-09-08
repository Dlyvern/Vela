#ifndef VELA_CORE_BACKEND_CONTEXT_IMPL_HPP
#define VELA_CORE_BACKEND_CONTEXT_IMPL_HPP

#include "volk.h"
#include "Vela/Core/IWindowBackend.hpp"
#include "Vela/Core/ContextPreferences.hpp"
#include "Vela/Core/MemoryStats.hpp"
#include "Vela/Core/DeviceInfo.hpp"

#include "vk_mem_alloc.h"

#include "Pipeline.hpp"
#include "DescriptorPool.hpp"

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

        LayoutCache& getLayoutCache();
        DescriptorPool& getDescriptorPool();
        VkDescriptorSetLayout getPerViewDescriptorSetLayout() const;

        const std::vector<VkImage>& getSwapchainImages() const;
        const std::vector<VkImageView>& getSwapchainImageViews() const;
        
        VkExtent2D getSwapchainExtent() const;
        VkFormat getSwapchainFormat() const;

        bool isSwapchainStale() const;

        void pickPhysicalDevice();
        void createDevice();
        void createSwapchain();
        void createSwapchainImageViews();
        void createCommandPool();
        void createAllocator();
        void recreateSwapchain();

        core::MemoryStats getMemoryStats() const;
        const core::DeviceInfo& getDeviceInfo() const;
        core::SwapchainInfo getSwapchainInfo() const;

        const VkPhysicalDeviceProperties& getPhysicalDeviceProperties() const;
        uint32_t getGraphicsTimestampValidBits() const;

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

        void buildDeviceInfo(const VkPhysicalDeviceDriverProperties& driverProperties);
        void logSelectedDevice() const;

        VkExtent2D framebufferExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

        bool checkInstanceExtensions(const std::vector<const char*>& extensions);
        bool checkDeviceExtension(VkPhysicalDevice physicalDevice, const std::string& extensionName);
        bool checkValidationLayers(const std::vector<const char*>& requiredLayers);

        //TODO expose it to public API later
        static constexpr uint32_t m_vulkanApiVersion{VK_API_VERSION_1_3};

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
        VkPresentModeKHR m_swapchainPresentMode{VK_PRESENT_MODE_FIFO_KHR};

        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;

        VkCommandPool m_commandPool{VK_NULL_HANDLE};

        core::IWindowBackend& m_windowBackend;

        VmaAllocator m_allocator{VK_NULL_HANDLE};

        VkPhysicalDeviceMemoryProperties m_physicalDeviceMemoryProperties;
        VkPhysicalDeviceProperties m_physicalDeviceProperties;
        core::DeviceInfo m_deviceInfo;
        uint32_t m_graphicsTimestampValidBits{0};
        bool m_memoryBudgetEnabled{false};

        std::optional<LayoutCache> m_layoutCache;
        std::optional<DescriptorPool> m_descriptorPool;
        VkDescriptorSetLayout m_perViewDescriptorSetLayout{VK_NULL_HANDLE};
    };

} // namespace vela::backend

#endif //VELA_CORE_BACKEND_CONTEXT_IMPL_HPP