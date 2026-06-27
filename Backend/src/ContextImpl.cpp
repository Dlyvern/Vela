#include "ContextImpl.hpp"
#include <vector>
#include <stdexcept>
#include <limits>
#include <fstream>

#include "Vela/Utility/Resources.hpp"
#include "Vela/Graphics/Vertex.hpp"

namespace vela::backend
{ 
    ContextImpl::ContextImpl(core::IWindowBackend& windowBackend) : m_windowBackend(windowBackend)
    {
        volkInitialize();
        createInstance(windowBackend);
        volkLoadInstance(m_instance);
    }

    ContextImpl::~ContextImpl()
    {
        vkDeviceWaitIdle(m_device);

        vmaDestroyAllocator(m_allocator);

        for (auto& imageView : m_swapchainImageViews) 
            vkDestroyImageView(m_device, imageView, nullptr);

        vkDestroyCommandPool(m_device, m_commandPool, nullptr);

        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

        vkDestroyDevice(m_device, nullptr);
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

        vkDestroyInstance(m_instance, nullptr);
    }

    VmaAllocator ContextImpl::getAllocator() const
    {
        return m_allocator;
    }

    void ContextImpl::createAllocator()
    {
        VmaVulkanFunctions vkFuncs{};
        vkFuncs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vkFuncs.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocInfo{};
        allocInfo.instance = m_instance;
        allocInfo.physicalDevice = m_physicalDevice;
        allocInfo.device = m_device;
        allocInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocInfo.pVulkanFunctions = &vkFuncs;

        if (vmaCreateAllocator(&allocInfo, &m_allocator) != VK_SUCCESS)
            throw std::runtime_error("Failed to create VMA allocator");
    }

    void ContextImpl::recreateSwapchain()
    {
        vkDeviceWaitIdle(m_device);

        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);

        while (caps.currentExtent.width == 0 || caps.currentExtent.height == 0)
        {
            m_windowBackend.waitEvents();
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);
        }

        vkDestroyImageView(m_device, m_depthImageView, nullptr);
        vmaDestroyImage(m_allocator, m_depthImage, m_depthImageAllocation);

        for (auto v  : m_swapchainImageViews) 
            vkDestroyImageView(m_device, v, nullptr);

        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

        m_swapchainImageViews.clear();

        createSwapchain();
        createSwapchainImageViews();
        createDepthImage();
    }

    void ContextImpl::createCommandPool()
    {
        VkCommandPoolCreateInfo commandPoolCI{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        commandPoolCI.queueFamilyIndex = m_graphicsFamily;
        commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if(VkResult result = vkCreateCommandPool(m_device, &commandPoolCI, nullptr, &m_commandPool); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool");
    }

    VkFormat ContextImpl::getSwapchainFormat() const
    {
        return m_swapchainFormat;
    }

    void ContextImpl::createSwapchain()
    {
        auto getSwapchainFormat = [this]
        {
            uint32_t surfaceFormatCount{0};
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &surfaceFormatCount, nullptr);
            std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &surfaceFormatCount, surfaceFormats.data());

            for(const VkSurfaceFormatKHR &format : surfaceFormats)
            {
                if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
                    return format;
            }

            return surfaceFormats[0];
        };

        auto getSwapchainPresentMode = [this]
        {
            uint32_t presentModeCount{0};
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);
            std::vector<VkPresentModeKHR> presentModes(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, presentModes.data());

            for(const VkPresentModeKHR presentMode : presentModes)
                if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                    return presentMode;

            return VK_PRESENT_MODE_FIFO_KHR;
        };

        auto getSwapchainExtent = [this]
        {
            VkSurfaceCapabilitiesKHR surfaceCapabilities;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfaceCapabilities);
            return surfaceCapabilities;
        };

        VkSurfaceFormatKHR format = getSwapchainFormat();
        VkPresentModeKHR presentMode = getSwapchainPresentMode();
        VkSurfaceCapabilitiesKHR surfaceCapabilities = getSwapchainExtent();

        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
        if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
            imageCount = surfaceCapabilities.maxImageCount;


        VkSwapchainCreateInfoKHR swapchainCI{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        swapchainCI.surface = m_surface;
        swapchainCI.minImageCount = imageCount;
        swapchainCI.imageFormat = format.format; 
        swapchainCI.imageColorSpace = format.colorSpace; 
        swapchainCI.imageExtent = surfaceCapabilities.currentExtent;
        swapchainCI.imageArrayLayers = 1;
        swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCI.preTransform = surfaceCapabilities.currentTransform;
        swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCI.presentMode  = presentMode;
        swapchainCI.clipped = VK_TRUE;
        swapchainCI.oldSwapchain = VK_NULL_HANDLE;

        if(VkResult result = vkCreateSwapchainKHR(m_device, &swapchainCI, nullptr, &m_swapchain); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create swapchain");

        m_swapchainFormat = format.format;
        m_swapchainExtent = surfaceCapabilities.currentExtent;

        uint32_t swapchainImagesCount;
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImagesCount, nullptr);
        m_swapchainImages.resize(swapchainImagesCount);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImagesCount, m_swapchainImages.data());
    }

    void ContextImpl::createSwapchainImageViews()
    {
        m_swapchainImageViews.resize(m_swapchainImages.size());

        for(int index = 0; index < m_swapchainImages.size(); ++index)
        {
            VkImageViewCreateInfo imageViewCI{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            imageViewCI.format = m_swapchainFormat;
            imageViewCI.image = m_swapchainImages[index];
            imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCI.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                      VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
            imageViewCI.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            if (VkResult result = vkCreateImageView(m_device, &imageViewCI, nullptr, &m_swapchainImageViews[index]);
                    result != VK_SUCCESS)
                throw std::runtime_error("Failed to create image view");
        }
    }

    void ContextImpl::createDepthImage()
    {
        VkImageCreateInfo imageCI{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.extent = {static_cast<uint32_t>(m_swapchainExtent.width), static_cast<uint32_t>(m_swapchainExtent.height), 1};
        imageCI.mipLevels = 1;
        imageCI.arrayLayers = 1;
        imageCI.format = VK_FORMAT_D32_SFLOAT;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL; 
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo imageAllocCI{};
        imageAllocCI.usage = VMA_MEMORY_USAGE_AUTO;

        if (vmaCreateImage(m_allocator, &imageCI, &imageAllocCI, &m_depthImage, &m_depthImageAllocation, nullptr) != VK_SUCCESS)
            throw std::runtime_error("Failed to create depth image");

        VkImageViewCreateInfo imageViewCI{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        imageViewCI.format = VK_FORMAT_D32_SFLOAT;
        imageViewCI.image = m_depthImage;
        imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCI.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                      VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        imageViewCI.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

        if (VkResult result = vkCreateImageView(m_device, &imageViewCI, nullptr, &m_depthImageView);
                result != VK_SUCCESS)
            throw std::runtime_error("Failed to create image view");
    }

    void ContextImpl::pickPhysicalDevice()
    {
        uint32_t physicalDevicesNum{0};
        vkEnumeratePhysicalDevices(m_instance, &physicalDevicesNum, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesNum);
        vkEnumeratePhysicalDevices(m_instance, &physicalDevicesNum, physicalDevices.data());

        if(physicalDevices.empty())
            throw std::runtime_error("Failed to find GPUs with Vulkan support");

        auto checkQueueFamilyProperties = [this](VkPhysicalDevice device)
        {
            uint32_t queueFamilyCount{0};
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamiles(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamiles.data());

            for(int index = 0; index < queueFamilyCount; ++index)
            {
                VkBool32 presentSupport{VK_FALSE};
                const auto &queueFamily = queueFamiles[index];

                vkGetPhysicalDeviceSurfaceSupportKHR(device, index, m_surface, &presentSupport);

                if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
                {
                    m_graphicsFamily = index;
                    return true;
                }
            }

            return false;
        };

        for(const auto& physicalDevice : physicalDevices)
            if(checkQueueFamilyProperties(physicalDevice))
            {
                m_physicalDevice = physicalDevice;
                return;
            }

        if (!m_physicalDevice)
            throw std::runtime_error("No suitable physical device found");
    }

    void ContextImpl::createDevice()
    {
        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCI{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueCI.queueFamilyIndex = m_graphicsFamily;
        queueCI.queueCount = 1;
        queueCI.pQueuePriorities = &queuePriority;

        std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        VkPhysicalDeviceVulkan13Features features13{};
        features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features13.dynamicRendering = VK_TRUE;

        VkDeviceCreateInfo deviceCI{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        deviceCI.pNext = &features13;
        deviceCI.queueCreateInfoCount = 1;
        deviceCI.pQueueCreateInfos = &queueCI;
        deviceCI.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCI.ppEnabledExtensionNames = deviceExtensions.data();

        if (vkCreateDevice(m_physicalDevice, &deviceCI, nullptr, &m_device) != VK_SUCCESS)
            throw std::runtime_error("Failed to create Vulkan device");

        volkLoadDevice(m_device);

        vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);
    }

    VkImage ContextImpl::getDepthImage() const
    {
        return m_depthImage;
    }

    VkImageView ContextImpl::getDepthImageView() const
    {
        return m_depthImageView;
    }

    VkExtent2D ContextImpl::getSwapchainExtent() const
    {
        return m_swapchainExtent;
    }

    VkSwapchainKHR ContextImpl::getSwapchain() const
    {
        return m_swapchain;
    }

    const std::vector<VkImage>& ContextImpl::getSwapchainImages() const
    {
        return m_swapchainImages;
    }

    const std::vector<VkImageView>& ContextImpl::getSwapchainImageViews() const
    {
        return m_swapchainImageViews;
    }

    VkCommandPool ContextImpl::getGraphicsCommandPool() const
    {
        return m_commandPool;
    }

    VkDevice ContextImpl::getDevice() const
    {
        return m_device;
    }
    
    VkInstance ContextImpl::getInstance() const
    {
        return m_instance;
    }

    VkQueue ContextImpl::getGraphicsQueue() const
    {
        return m_graphicsQueue;
    }

    void ContextImpl::setSurface(VkSurfaceKHR surface)
    {
        m_surface = surface;
    }

    void ContextImpl::createInstance(core::IWindowBackend& windowBackend)
    {
        VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        appInfo.pApplicationName = "Vela";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.pEngineName = "VelaEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        std::vector<const char*> extensions = windowBackend.requiredInstanceExtensions();

#ifdef VELA_DEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        std::vector<const char*> layers;

        #ifdef VELA_DEBUG
            layers.push_back("VK_LAYER_KHRONOS_validation");
        #endif

        VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames = layers.data();

        if(VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create Vulkan instance");
    }
} //namespace vela::backend
