#ifndef VELA_CORE_DEVICE_INFO_HPP
#define VELA_CORE_DEVICE_INFO_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "Feature.hpp"
#include "ContextPreferences.hpp"

namespace vela::core
{
    enum class DeviceType : uint8_t
    {
        Other = 0,
        IntegratedGpu,
        DiscreteGpu,
        VirtualGpu,
        Cpu
    };

    struct DeviceInfo
    {
        std::string name;
        DeviceType type{DeviceType::Other};

        uint32_t vendorId{0};
        uint32_t deviceId{0};

        std::string driverName;
        std::string driverInfo;

        Version apiVersion{};

        uint64_t deviceLocalMemoryBytes{0};
        uint64_t hostVisibleMemoryBytes{0};
        bool unifiedMemory{false};

        bool timestampsSupported{false};
        float timestampPeriodNs{0.0f};

        uint32_t maxTextureSize2D{0};
        uint32_t maxMsaaSamples{1};
        float maxAnisotropy{1.0f};

        std::vector<Feature> supportedFeatures;
        std::vector<Feature> enabledFeatures;

        // what the device can do
        bool supports(Feature feature) const
        {
            for (Feature supported : supportedFeatures)
                if (supported == feature)
                    return true;

            return false;
        }

        // what you actually turned on
        bool enabled(Feature feature) const
        {
            for (Feature active : enabledFeatures)
                if (active == feature)
                    return true;

            return false;
        }
    };

    struct SwapchainInfo
    {
        uint32_t imageCount{0};
        uint32_t width{0};
        uint32_t height{0};
        VSync requestedVSync{VSync::On};
        VSync actualVSync{VSync::On};
    };

    constexpr const char* toString(DeviceType type)
    {
        switch (type)
        {
            case DeviceType::IntegratedGpu: return "Integrated GPU";
            case DeviceType::DiscreteGpu:   return "Discrete GPU";
            case DeviceType::VirtualGpu:    return "Virtual GPU";
            case DeviceType::Cpu:           return "CPU";
            case DeviceType::Other:         return "Other";
        }

        return "Other";
    }

    constexpr const char* toString(VSync vsync)
    {
        switch (vsync)
        {
            case VSync::Off:      return "Off (immediate)";
            case VSync::On:       return "On (fifo)";
            case VSync::Adaptive: return "Adaptive (fifo relaxed)";
            case VSync::Fast:     return "Fast (mailbox)";
        }

        return "On (fifo)";
    }
} //namespace vela::core

#endif //VELA_CORE_DEVICE_INFO_HPP
