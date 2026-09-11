#include "ContextImpl.hpp"
#include <array>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <unordered_set>

namespace vela::backend
{ 
    ContextImpl::ContextImpl(core::IWindowBackend& windowBackend, const core::ContextPreferences& contextPreferences) : m_windowBackend(windowBackend),
    m_contextPreferences(contextPreferences)
    {
        volkInitialize();
        createInstance(windowBackend);
        volkLoadInstance(m_instance);
    }

    ContextImpl::~ContextImpl()
    {
        if (m_device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);

            if (m_deletionQueue)
                m_deletionQueue->flushAll();

            // After every allocation it owns, never before.
            if (m_allocator != VK_NULL_HANDLE)
                vmaDestroyAllocator(m_allocator);

            for (auto& imageView : m_swapchainImageViews)
                vkDestroyImageView(m_device, imageView, nullptr);

            vkDestroyCommandPool(m_device, m_commandPool, nullptr);

            vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

            m_descriptorPool.reset();
            m_layoutCache.reset();

            vkDestroyDevice(m_device, nullptr);
        }

        if (m_surface != VK_NULL_HANDLE)
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

        if (m_instance != VK_NULL_HANDLE)
            vkDestroyInstance(m_instance, nullptr);
    }

    VmaAllocator ContextImpl::getAllocator() const
    {
        return m_allocator;
    }

    const VkPhysicalDeviceProperties& ContextImpl::getPhysicalDeviceProperties() const
    {
        return m_physicalDeviceProperties;
    }

    DeletionQueue& ContextImpl::getDeletionQueue()
    {
        return *m_deletionQueue;
    }

    uint32_t ContextImpl::getGraphicsTimestampValidBits() const
    {
        return m_graphicsTimestampValidBits;
    }

    core::MemoryStats ContextImpl::getMemoryStats() const
    {
        std::array<VmaBudget, VK_MAX_MEMORY_HEAPS> budgets{};
        vmaGetHeapBudgets(m_allocator, budgets.data());

        core::MemoryStats memoryStats{};
        memoryStats.budgetFromDriver = m_memoryBudgetEnabled;

        for (uint32_t heap = 0; heap < m_physicalDeviceMemoryProperties.memoryHeapCount; ++heap)
        {
            if ((m_physicalDeviceMemoryProperties.memoryHeaps[heap].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == 0)
                continue;

            memoryStats.deviceBytesUsed += budgets[heap].usage;
            memoryStats.deviceBytesBudget += budgets[heap].budget;
            memoryStats.deviceBytesAllocated += budgets[heap].statistics.allocationBytes;
            memoryStats.allocationCount += budgets[heap].statistics.allocationCount;
        }

        return memoryStats;
    }

    void ContextImpl::createAllocator()
    {
        VmaVulkanFunctions vkFuncs{};
        vkFuncs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vkFuncs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocInfo{};
        allocInfo.instance = m_instance;
        allocInfo.physicalDevice = m_physicalDevice;
        allocInfo.device = m_device;
        allocInfo.vulkanApiVersion = m_vulkanApiVersion;
        allocInfo.pVulkanFunctions = &vkFuncs;

        m_memoryBudgetEnabled = checkDeviceExtension(m_physicalDevice, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);

        if(m_memoryBudgetEnabled)
        {
            allocInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
            std::cout << "Driver does have VK_EXT_MEMORY_BUDGET_EXTENSION_NAME extension\n";
        }
        else
            std::cerr << "Driver does not have VK_EXT_MEMORY_BUDGET_EXTENSION_NAME extension\n";

        if (vmaCreateAllocator(&allocInfo, &m_allocator) != VK_SUCCESS)
            throw std::runtime_error("Failed to create VMA allocator");

        m_deletionQueue = std::make_unique<DeletionQueue>(m_device, m_allocator);
    }

    void ContextImpl::recreateSwapchain()
    {
        vkDeviceWaitIdle(m_device);

        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);

        // Minimised: block until there is something to render into again.
        while (framebufferExtent(caps).width == 0 || framebufferExtent(caps).height == 0)
        {
            m_windowBackend.waitEvents();
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);
        }

        for (auto v  : m_swapchainImageViews)
            vkDestroyImageView(m_device, v, nullptr);

        m_swapchainImageViews.clear();

        createSwapchain();
        createSwapchainImageViews();
    }

    void ContextImpl::createCommandPool()
    {
        VkCommandPoolCreateInfo commandPoolCI{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        commandPoolCI.queueFamilyIndex = m_queueFamilyIndices.graphics.value();
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

            auto toVkPresentMode = [](core::VSync vsync)
            {
                switch (vsync)
                {
                    case core::VSync::Off:      return VK_PRESENT_MODE_IMMEDIATE_KHR;
                    case core::VSync::Fast:     return VK_PRESENT_MODE_MAILBOX_KHR;
                    case core::VSync::Adaptive: return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                    default:                    return VK_PRESENT_MODE_FIFO_KHR;
                }
            };

            const VkPresentModeKHR preferred = toVkPresentMode(m_contextPreferences.preferredVSync);

            for(const VkPresentModeKHR presentMode : presentModes)
                if(presentMode == preferred)
                    return preferred;

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

        m_swapchainPresentMode = presentMode;

        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
        if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
            imageCount = surfaceCapabilities.maxImageCount;

        VkExtent2D extent = framebufferExtent(surfaceCapabilities);

        VkImageUsageFlags wantedImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        if ((surfaceCapabilities.supportedUsageFlags & wantedImageUsage) != wantedImageUsage)
            throw std::runtime_error("Surface does not support required swapchain image usage");

        VkSwapchainCreateInfoKHR swapchainCI{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        swapchainCI.surface = m_surface;
        swapchainCI.minImageCount = imageCount;
        swapchainCI.imageFormat = format.format; 
        swapchainCI.imageColorSpace = format.colorSpace; 
        swapchainCI.imageExtent = extent;
        swapchainCI.imageArrayLayers = 1;
        swapchainCI.imageUsage = wantedImageUsage;
        swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCI.preTransform = surfaceCapabilities.currentTransform;
        swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCI.presentMode  = presentMode;
        swapchainCI.clipped = VK_TRUE;
        swapchainCI.oldSwapchain = m_swapchain;

        VkSwapchainKHR oldSwapchain = m_swapchain;

        VkResult swapchainCreationResult = vkCreateSwapchainKHR(m_device, &swapchainCI, nullptr, &m_swapchain);

        if(swapchainCreationResult != VK_SUCCESS)
        {
            std::cerr << "Swapchain error result: " << swapchainCreationResult << '\n';
            throw std::runtime_error("Failed to create swapchain");
        }

        if (oldSwapchain != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);

        m_swapchainFormat = format.format;
        m_swapchainExtent = extent;

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

    void ContextImpl::pickPhysicalDevice()
    {
        uint32_t physicalDevicesNum{0};
        vkEnumeratePhysicalDevices(m_instance, &physicalDevicesNum, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesNum);
        vkEnumeratePhysicalDevices(m_instance, &physicalDevicesNum, physicalDevices.data());

        if(physicalDevices.empty())
            throw std::runtime_error("Failed to find GPUs with Vulkan support");

        auto gpuTypeToVk = [](core::GpuPreference preference)
        {
            if(preference == core::GpuPreference::Discrete)
                return VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
            if(preference == core::GpuPreference::Integrated)
                return VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

            return VK_PHYSICAL_DEVICE_TYPE_OTHER;
        };

        auto gpuPreferenceName = [](core::GpuPreference preference)
        {
            return preference == core::GpuPreference::Discrete ? "discrete" : "integrated";
        };

        auto checkQueueFamilyProperties = [this](VkPhysicalDevice device)
        {
            QueueFamilyIndices indices;
            uint32_t queueFamilyCount{0};
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamiles(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamiles.data());

            // std::cout << queueFamilyCount << '\n';

            for(int index = 0; index < queueFamilyCount; ++index)
            {
                const auto &queueFamily = queueFamiles[index];
                
                // Graphics
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    if (!indices.graphics)
                    {
                        indices.graphics = index;
                        // std::cout << "Graphics\n";
                    }
                }

                // Dedicated compute queue:
                // compute but NOT graphics 
                if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
                {
                    if (!indices.compute)
                    {
                        indices.compute = index;
                        // std::cout << "Compute\n";

                    }
                }

                // Dedicated transfer queue:
                // transfer but NOT graphics/compute
                if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) &&  !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                    !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
                {
                    if (!indices.transfer)
                    {
                        indices.transfer = index;
                        // std::cout << "Transfer\n";
                    }
                }

                // Present support

                VkBool32 presentSupport{VK_FALSE};
                vkGetPhysicalDeviceSurfaceSupportKHR(device, index, m_surface, &presentSupport);

                if (presentSupport && !indices.present)
                {
                    indices.present = index;
                    // std::cout << "Present\n";
                }
            }

            // Fallbacks
            if (!indices.compute)
            {
                for (uint32_t i = 0; i < queueFamilyCount; ++i)
                {
                    if (queueFamiles[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                    {
                        indices.compute = i;
                        break;
                    }
                }
            }

            if (!indices.transfer)
            {
                for (uint32_t i = 0; i < queueFamilyCount; ++i)
                {
                    if (queueFamiles[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
                    {
                        indices.transfer = i;
                        break;
                    }
                }
            }

            if(!indices.complete())
                return false;

            m_queueFamilyIndices = indices;

            return true;
        };

        const VkPhysicalDeviceType preferredType = gpuTypeToVk(m_contextPreferences.preferredGpu);

        VkPhysicalDevice fallbackDevice{VK_NULL_HANDLE};
        VkPhysicalDeviceProperties fallbackProperties{};
        QueueFamilyIndices fallbackIndices;

        for(const auto& physicalDevice : physicalDevices)
        {
            VkPhysicalDeviceProperties physicalDeviceProperties{};
            vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

            if(!checkQueueFamilyProperties(physicalDevice))
                continue;

            if(!supportsRequiredFeatures(physicalDevice))
                continue;

            if(physicalDeviceProperties.deviceType == preferredType)
            {
                m_physicalDevice = physicalDevice;
                break;
            }

            if(fallbackDevice == VK_NULL_HANDLE)
            {
                fallbackDevice = physicalDevice;
                fallbackProperties = physicalDeviceProperties;
                fallbackIndices = m_queueFamilyIndices;
            }
        }

        if (!m_physicalDevice)
        {
            if (fallbackDevice == VK_NULL_HANDLE)
            {
                for (core::Feature feature : m_contextPreferences.requiredFeatures)
                {
                    bool anyDeviceSupports = false;

                    for (const auto& physicalDevice : physicalDevices)
                        if (deviceSupportsFeature(physicalDevice, feature))
                        {
                            anyDeviceSupports = true;
                            break;
                        }

                    if (!anyDeviceSupports)
                        throw std::runtime_error(std::string("No device supports Feature::")
                            + core::toString(feature));
                }

                throw std::runtime_error("No suitable physical device found");
            }

            m_physicalDevice = fallbackDevice;
            m_queueFamilyIndices = fallbackIndices;

            std::cerr << "No " << gpuPreferenceName(m_contextPreferences.preferredGpu)
                      << " GPU available, falling back to \"" << fallbackProperties.deviceName << "\"\n";
        }

        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_physicalDeviceMemoryProperties);

        VkPhysicalDeviceDriverProperties driverProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES};

        VkPhysicalDeviceProperties2 physicalDeviceProperties2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        physicalDeviceProperties2.pNext = &driverProperties;

        vkGetPhysicalDeviceProperties2(m_physicalDevice, &physicalDeviceProperties2);

        m_physicalDeviceProperties = physicalDeviceProperties2.properties;

        uint32_t queueFamilyCount{0};
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

        m_graphicsTimestampValidBits = queueFamilies[m_queueFamilyIndices.graphics.value()].timestampValidBits;

        resolveEnabledFeatures();

        buildDeviceInfo(driverProperties);
        logSelectedDevice();
    }

    bool ContextImpl::supportsRequiredFeatures(VkPhysicalDevice physicalDevice) const
    {
        for (core::Feature feature : m_contextPreferences.requiredFeatures)
            if (!deviceSupportsFeature(physicalDevice, feature))
                return false;

        return true;
    }

    void ContextImpl::resolveEnabledFeatures()
    {
        m_enabledFeatures.clear();

        auto alreadyEnabled = [this](core::Feature feature)
        {
            return std::find(m_enabledFeatures.begin(), m_enabledFeatures.end(), feature) != m_enabledFeatures.end();
        };

        for (core::Feature feature : m_contextPreferences.requiredFeatures)
            if (!alreadyEnabled(feature))
                m_enabledFeatures.push_back(feature);

        for (core::Feature feature : m_contextPreferences.optionalFeatures)
            if (!alreadyEnabled(feature) && deviceSupportsFeature(m_physicalDevice, feature))
                m_enabledFeatures.push_back(feature);
    }

    void ContextImpl::buildDeviceInfo(const VkPhysicalDeviceDriverProperties& driverProperties)
    {
        const VkPhysicalDeviceLimits& limits = m_physicalDeviceProperties.limits;

        m_deviceInfo = core::DeviceInfo{};

        m_deviceInfo.name = m_physicalDeviceProperties.deviceName;

        switch (m_physicalDeviceProperties.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: m_deviceInfo.type = core::DeviceType::IntegratedGpu; break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   m_deviceInfo.type = core::DeviceType::DiscreteGpu;   break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    m_deviceInfo.type = core::DeviceType::VirtualGpu;    break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:            m_deviceInfo.type = core::DeviceType::Cpu;           break;
            default:                                     m_deviceInfo.type = core::DeviceType::Other;         break;
        }

        m_deviceInfo.vendorId = m_physicalDeviceProperties.vendorID;
        m_deviceInfo.deviceId = m_physicalDeviceProperties.deviceID;

        m_deviceInfo.driverName = driverProperties.driverName;
        m_deviceInfo.driverInfo = driverProperties.driverInfo;

        m_deviceInfo.apiVersion = core::Version{
            VK_API_VERSION_MAJOR(m_physicalDeviceProperties.apiVersion),
            VK_API_VERSION_MINOR(m_physicalDeviceProperties.apiVersion),
            VK_API_VERSION_PATCH(m_physicalDeviceProperties.apiVersion)};

        std::array<bool, VK_MAX_MEMORY_HEAPS> hostVisibleHeaps{};

        for (uint32_t type = 0; type < m_physicalDeviceMemoryProperties.memoryTypeCount; ++type)
        {
            const VkMemoryType& memoryType = m_physicalDeviceMemoryProperties.memoryTypes[type];

            if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                hostVisibleHeaps[memoryType.heapIndex] = true;
        }

        for (uint32_t heap = 0; heap < m_physicalDeviceMemoryProperties.memoryHeapCount; ++heap)
        {
            const VkMemoryHeap& memoryHeap = m_physicalDeviceMemoryProperties.memoryHeaps[heap];

            if (memoryHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                m_deviceInfo.deviceLocalMemoryBytes += memoryHeap.size;

            if (hostVisibleHeaps[heap])
                m_deviceInfo.hostVisibleMemoryBytes += memoryHeap.size;
        }

        m_deviceInfo.unifiedMemory = m_deviceInfo.type == core::DeviceType::IntegratedGpu
                                  || m_deviceInfo.type == core::DeviceType::Cpu;

        for (core::Feature feature : allFeatures())
            if (deviceSupportsFeature(m_physicalDevice, feature))
                m_deviceInfo.supportedFeatures.push_back(feature);

        m_deviceInfo.enabledFeatures = m_enabledFeatures;

        m_deviceInfo.timestampsSupported = m_graphicsTimestampValidBits != 0 && limits.timestampPeriod != 0.0f;
        m_deviceInfo.timestampPeriodNs = limits.timestampPeriod;

        m_deviceInfo.maxTextureSize2D = limits.maxImageDimension2D;
        m_deviceInfo.maxAnisotropy = limits.maxSamplerAnisotropy;

        const VkSampleCountFlags sampleCounts = limits.framebufferColorSampleCounts
                                              & limits.framebufferDepthSampleCounts;

        for (const uint32_t samples : {64u, 32u, 16u, 8u, 4u, 2u})
        {
            if (sampleCounts & samples)
            {
                m_deviceInfo.maxMsaaSamples = samples;
                break;
            }
        }
    }

    void ContextImpl::logSelectedDevice() const
    {
        constexpr uint64_t megabyte = 1024 * 1024;

        std::cout << "Selected GPU: \n------------------------------------------\n";

        std::cout << "Name: " << m_deviceInfo.name << '\n';
        std::cout << "Type: " << core::toString(m_deviceInfo.type) << '\n';
        std::cout << "Vendor / device id: 0x" << std::hex << m_deviceInfo.vendorId
                  << " / 0x" << m_deviceInfo.deviceId << std::dec << '\n';
        std::cout << "Driver: " << m_deviceInfo.driverName << " (" << m_deviceInfo.driverInfo << ")\n";
        std::cout << "API version: " << m_deviceInfo.apiVersion.major << '.'
                  << m_deviceInfo.apiVersion.minor << '.' << m_deviceInfo.apiVersion.patch << '\n';
        std::cout << "Device local memory: " << m_deviceInfo.deviceLocalMemoryBytes / megabyte << " MB\n";
        std::cout << "Host visible memory: " << m_deviceInfo.hostVisibleMemoryBytes / megabyte << " MB\n";
        std::cout << "Unified memory: " << (m_deviceInfo.unifiedMemory ? "yes" : "no") << '\n';
        std::cout << "Timestamps: " << (m_deviceInfo.timestampsSupported ? "yes" : "no")
                  << " (period " << m_deviceInfo.timestampPeriodNs << " ns)\n";
        std::cout << "Max 2D texture size: " << m_deviceInfo.maxTextureSize2D << '\n';
        std::cout << "Max MSAA samples: " << m_deviceInfo.maxMsaaSamples << '\n';
        std::cout << "Max anisotropy: " << m_deviceInfo.maxAnisotropy << '\n';

        std::cout << "Supported features:";
        for (core::Feature feature : m_deviceInfo.supportedFeatures)
            std::cout << ' ' << core::toString(feature);
        std::cout << '\n';

        std::cout << "Enabled features:";
        for (core::Feature feature : m_deviceInfo.enabledFeatures)
            std::cout << ' ' << core::toString(feature);
        std::cout << '\n';
        std::cout << "Present queue family " << m_queueFamilyIndices.present.value() << '\n';
        std::cout << "Graphics queue family " << m_queueFamilyIndices.graphics.value() << '\n';
        std::cout << "Transfer queue family " << m_queueFamilyIndices.transfer.value() << '\n';
        std::cout << "Compute queue family " << m_queueFamilyIndices.compute.value() << '\n';

        std::cout << "------------------------------------------\n";
    }

    const core::DeviceInfo& ContextImpl::getDeviceInfo() const
    {
        return m_deviceInfo;
    }

    core::SwapchainInfo ContextImpl::getSwapchainInfo() const
    {
        auto toVSync = [](VkPresentModeKHR presentMode)
        {
            switch (presentMode)
            {
                case VK_PRESENT_MODE_IMMEDIATE_KHR:    return core::VSync::Off;
                case VK_PRESENT_MODE_MAILBOX_KHR:      return core::VSync::Fast;
                case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return core::VSync::Adaptive;
                default:                               return core::VSync::On;
            }
        };

        core::SwapchainInfo swapchainInfo{};
        swapchainInfo.imageCount = static_cast<uint32_t>(m_swapchainImages.size());
        swapchainInfo.width = m_swapchainExtent.width;
        swapchainInfo.height = m_swapchainExtent.height;
        swapchainInfo.requestedVSync = m_contextPreferences.preferredVSync;
        swapchainInfo.actualVSync = toVSync(m_swapchainPresentMode);

        return swapchainInfo;
    }

    void ContextImpl::createDevice()
    {
        float queuePriority = 1.0f;

        std::vector<VkDeviceQueueCreateInfo> queueInfos;

        std::unordered_set<uint32_t> uniqueFamilies = {
            m_queueFamilyIndices.graphics.value(),
            m_queueFamilyIndices.compute.value(),
            m_queueFamilyIndices.transfer.value(),
        };

        for (const auto& family : uniqueFamilies)
        {
            VkDeviceQueueCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            info.queueFamilyIndex = family;
            info.queueCount = 1;
            info.pQueuePriorities = &queuePriority;

            queueInfos.push_back(info);
        }

        std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        for (core::Feature feature : m_enabledFeatures)
        {
            for (const char* extensionName : featureExtensions(feature))
            {
                const bool alreadyRequested = std::any_of(deviceExtensions.begin(), deviceExtensions.end(),
                    [extensionName](const char* requested){ return std::strcmp(requested, extensionName) == 0; });

                if (!alreadyRequested)
                    deviceExtensions.push_back(extensionName);
            }
        }

        FeatureChain featureChain{};
        featureChain.features13.dynamicRendering = VK_TRUE;
        featureChain.features13.synchronization2 = VK_TRUE;

        for (core::Feature feature : m_enabledFeatures)
            featureChain.enable(feature);

        featureChain.link(m_enabledFeatures);

        VkDeviceCreateInfo deviceCI{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        deviceCI.pNext = &featureChain.features2;
        deviceCI.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
        deviceCI.pQueueCreateInfos = queueInfos.data();
        deviceCI.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCI.ppEnabledExtensionNames = deviceExtensions.data();

        if (vkCreateDevice(m_physicalDevice, &deviceCI, nullptr, &m_device) != VK_SUCCESS)
            throw std::runtime_error("Failed to create Vulkan device");

        volkLoadDevice(m_device);

        vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphics.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, m_queueFamilyIndices.compute.value(), 0, &m_computeQueue);
        vkGetDeviceQueue(m_device, m_queueFamilyIndices.transfer.value(), 0, &m_transferQueue);

        m_layoutCache.emplace(m_device);
        m_descriptorPool.emplace(m_device);

        VkDescriptorSetLayoutBinding perViewBinding{};
        perViewBinding.binding = 0;
        perViewBinding.descriptorCount = 1;
        perViewBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        perViewBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        m_perViewDescriptorSetLayout = m_layoutCache->getDescriptorSetLayout({perViewBinding});
    }

    LayoutCache& ContextImpl::getLayoutCache()
    {
        return *m_layoutCache;
    }

    DescriptorPool& ContextImpl::getDescriptorPool()
    {
        return *m_descriptorPool;
    }

    VkDescriptorSetLayout ContextImpl::getPerViewDescriptorSetLayout() const
    {
        return m_perViewDescriptorSetLayout;
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

    void ContextImpl::setNativeWindow(void* nativeWindow)
    {
        m_nativeWindow = nativeWindow;
    }

    void ContextImpl::waitIdle() const
    {
        vkDeviceWaitIdle(m_device);
    }

    bool ContextImpl::isSwapchainStale() const
    {
        int width{0};
        int height{0};
        m_windowBackend.getFramebufferSize(m_nativeWindow, width, height);

        if (static_cast<uint32_t>(width) == m_swapchainExtent.width
            && static_cast<uint32_t>(height) == m_swapchainExtent.height)
            return false;

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);

        const VkExtent2D extent = framebufferExtent(capabilities);

        return extent.width != m_swapchainExtent.width || extent.height != m_swapchainExtent.height;
    }

    VkExtent2D ContextImpl::framebufferExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
    {
        if (capabilities.currentExtent.width != UINT32_MAX && capabilities.currentExtent.height != UINT32_MAX)
            return capabilities.currentExtent;

        int width{0};
        int height{0};
        m_windowBackend.getFramebufferSize(m_nativeWindow, width, height);

        VkExtent2D extent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return extent;
    }

    bool ContextImpl::checkDeviceExtension(VkPhysicalDevice physicalDevice, const std::string& extensionName)
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

        for (const auto& extension : availableExtensions)
        {
            if (strcmp(extension.extensionName, extensionName.c_str()) == 0) 
            {
                return true;
            }
        }

        return false;
    }

    bool ContextImpl::checkInstanceExtensions(const std::vector<const char*>& extensions)
    {
        uint32_t extensionCount = 0;

        VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
            nullptr);

        if (result != VK_SUCCESS)
            return false;

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);

        result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

        if (result != VK_SUCCESS)
            return false;

        for (const char* required : extensions)
        {
            bool found = false;

            for (const auto& available : availableExtensions)
            {
                if (std::strcmp(required, available.extensionName) == 0)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                std::cerr << "Missing Vulkan instance extension: " << required << '\n';
                return false;
            }
        }

        return true;
    }

    bool ContextImpl::checkValidationLayers(const std::vector<const char*>& requiredLayers)
    {
        uint32_t layerCount = 0;

        VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        if (result != VK_SUCCESS)
            return false;

        std::vector<VkLayerProperties> availableLayers(layerCount);

        result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        if (result != VK_SUCCESS)
            return false;

        for (const char* required : requiredLayers)
        {
            bool found = false;

            for (const auto& available : availableLayers)
            {
                if (std::strcmp(required, available.layerName) == 0)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                std::cerr << "Missing Vulkan layer: " << required << '\n';
                return false;
            }
        }

        return true;
    }

    void ContextImpl::createInstance(core::IWindowBackend& windowBackend)
    {
        VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        appInfo.pApplicationName = m_contextPreferences.applicationName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(m_contextPreferences.applicationVersion.major, 
            m_contextPreferences.applicationVersion.minor, m_contextPreferences.applicationVersion.patch);
        
        appInfo.pEngineName = m_contextPreferences.engineName.c_str();
        appInfo.engineVersion = VK_MAKE_VERSION(m_contextPreferences.engineVersion.major, 
            m_contextPreferences.engineVersion.minor, m_contextPreferences.engineVersion.patch);

        appInfo.apiVersion = m_vulkanApiVersion;

        std::vector<const char*> extensions = windowBackend.requiredInstanceExtensions();

#ifdef VELA_DEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
        
        if(!checkInstanceExtensions(extensions))
            throw std::runtime_error("Required Vulkan instance extensions are unavailable");

        std::vector<const char*> layers;

#ifdef VELA_DEBUG
        layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

        if(!checkValidationLayers(layers))
            throw std::runtime_error("Required Vulkan layers are unavailable");

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
