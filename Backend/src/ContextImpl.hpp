#ifndef VELA_CORE_BACKEND_CONTEXT_IMPL_HPP
#define VELA_CORE_BACKEND_CONTEXT_IMPL_HPP

#include "volk.h"
#include "Vela/Core/IWindowBackend.hpp"
#include "Vela/Core/ContextPreferences.hpp"
#include "Vela/Core/MemoryStats.hpp"
#include "Vela/Core/DeviceInfo.hpp"
#include "Vela/Result.hpp"

#include "vk_mem_alloc.h"

#include "Pipeline.hpp"
#include "SamplerCache.hpp"
#include "DescriptorPool.hpp"
#include "DeletionQueue.hpp"
#include "Features.hpp"

#include <deque>
#include <optional>
#include <memory>

//TODO add a way to a public API to let people request extensions(Like for RTX for example)
//TODO Over-Recreating on Every Little Event: Avoid rebuilding the swap chain continuously while a user is actively dragging the window border.
// You can rate-limit resizes or wait until the resize action finishes. (Only on Linux, maybe worth doing at the end of development)

namespace vela::backend
{
    inline constexpr uint32_t k_passInputSet = 2;
    inline constexpr uint32_t k_passInputSlots = 4;

    class ContextImpl
    {
    public:
        ContextImpl(core::IWindowBackend& windowBackend, const core::ContextPreferences& contextPreferences);
        ~ContextImpl();

        void setSurface(VkSurfaceKHR surface);
        void setNativeWindow(void* nativeWindow);

        void waitIdle() const;
        Status releaseDestroyed();

        VkDevice getDevice() const;
        VkInstance getInstance() const;
        VkCommandPool getGraphicsCommandPool() const;
        VmaAllocator getAllocator() const;
        VkQueue getGraphicsQueue() const;
        VkSwapchainKHR getSwapchain() const;

        LayoutCache& getLayoutCache();
        SamplerCache& getSamplerCache();
        DescriptorPool& getDescriptorPool();
        VkDescriptorSetLayout getPerViewDescriptorSetLayout() const;
        VkDescriptorSetLayout getPassInputDescriptorSetLayout() const;

        const std::vector<VkImage>& getSwapchainImages() const;
        const std::vector<VkImageView>& getSwapchainImageViews() const;
        
        VkExtent2D getSwapchainExtent() const;
        VkFormat getSwapchainFormat() const;

        bool isSwapchainStale() const;

        // False while the window is minimised: the surface has a zero extent and no swapchain can be created for it
        bool isSurfaceDrawable() const;

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
        std::vector<core::VSync> getSupportedVSync() const;
        Status setVSync(core::VSync vsync);

        const VkPhysicalDeviceProperties& getPhysicalDeviceProperties() const;
        uint32_t getGraphicsTimestampValidBits() const;

        DeletionQueue& getDeletionQueue();

        VkPhysicalDevice getPhysicalDevice() const;

        /*
            Submits a recorded upload command buffer (from the graphics command pool) to the graphics queue without
            waiting for it. The command buffer and staging buffer are freed once the upload's fence signals.
            Later submissions on the queue are ordered after it by the barriers the upload records, so frames
            need no extra synchronization. Blocks only while more than k_maxPendingUploadBytes of staging is
            still in flight, to bound memory while loading many textures
        */
        Status submitUpload(VkCommandBuffer commandBuffer, VkBuffer stagingBuffer, VmaAllocation stagingAllocation,
            VkDeviceSize stagingBytes);

        // Frees uploads that finished. wait: block until every pending upload finished first
        void reclaimUploads(bool wait);

        uint32_t getValidationMessageCount() const;

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

        std::unique_ptr<DeletionQueue> m_deletionQueue{nullptr};

        struct PendingUpload
        {
            VkFence fence{VK_NULL_HANDLE};
            VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
            VkBuffer stagingBuffer{VK_NULL_HANDLE};
            VmaAllocation stagingAllocation{VK_NULL_HANDLE};
            VkDeviceSize stagingBytes{0};
        };

        static constexpr VkDeviceSize k_maxPendingUploadBytes{256ull * 1024 * 1024};
        static constexpr VkDeviceSize k_preferredLargeHeapBlockSize{64ull * 1024 * 1024};

        std::deque<PendingUpload> m_pendingUploads;
        VkDeviceSize m_pendingUploadBytes{0};

        void releaseUpload(const PendingUpload& upload);

        void createInstance(core::IWindowBackend& windowBackend);
        void createDebugMessenger();

        void buildDeviceInfo(const VkPhysicalDeviceDriverProperties& driverProperties);
        void logSelectedDevice() const;

        bool supportsRequiredFeatures(VkPhysicalDevice physicalDevice) const;
        void resolveEnabledFeatures();

        VkExtent2D framebufferExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

        bool checkInstanceExtensions(const std::vector<const char*>& extensions, const std::vector<const char*>& layers);
        bool checkDeviceExtension(VkPhysicalDevice physicalDevice, const std::string& extensionName);
        bool checkValidationLayers(const std::vector<const char*>& requiredLayers);

        //TODO expose it to public API later
        static constexpr uint32_t m_vulkanApiVersion{VK_API_VERSION_1_3};

        core::ContextPreferences m_contextPreferences;

        VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
        VkDevice m_device{VK_NULL_HANDLE};
        VkInstance m_instance{VK_NULL_HANDLE};
        VkDebugUtilsMessengerEXT m_debugMessenger{VK_NULL_HANDLE};
        bool m_syncValidationEnabled{false};
        static inline uint32_t m_validationMessageCount{0};
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

        // Set by setVSync(): the swapchain must be recreated with a new present mode
        bool m_presentModeChanged{false};
        bool m_fifoLatestReadyEnabled{false};

        std::vector<VkPresentModeKHR> surfacePresentModes() const;
        VkPresentModeKHR choosePresentMode() const;

        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;

        VkCommandPool m_commandPool{VK_NULL_HANDLE};

        core::IWindowBackend& m_windowBackend;

        VmaAllocator m_allocator{VK_NULL_HANDLE};

        VkPhysicalDeviceMemoryProperties m_physicalDeviceMemoryProperties;
        VkPhysicalDeviceProperties m_physicalDeviceProperties;
        core::DeviceInfo m_deviceInfo;
        std::vector<core::Feature> m_enabledFeatures;
        uint32_t m_graphicsTimestampValidBits{0};
        bool m_memoryBudgetEnabled{false};

        std::optional<LayoutCache> m_layoutCache;
        std::optional<SamplerCache> m_samplerCache;
        std::optional<DescriptorPool> m_descriptorPool;
        VkDescriptorSetLayout m_perViewDescriptorSetLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout m_passInputDescriptorSetLayout{VK_NULL_HANDLE};
    };

} // namespace vela::backend

#endif //VELA_CORE_BACKEND_CONTEXT_IMPL_HPP