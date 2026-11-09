#ifndef VELA_CORE_CONTEXT_PREFERENCES_HPP
#define VELA_CORE_CONTEXT_PREFERENCES_HPP

#include "Feature.hpp"

#include <string>
#include <cstdint>
#include <vector>

namespace vela::core
{
    enum class GpuPreference : uint8_t
    {
        Discrete = 0,
        Integrated
    };

    /*
        How frames are presented. Not every mode exists everywhere: Wayland, for example, has no Off.
        A missing mode falls back to the closest one that exists:
        Off -> Fast -> On, Fast -> On, Adaptive -> On, LatestReady -> On. On always exists.
        SwapchainInfo::actualVSync reports what was chosen, Context::getSupportedVSync() what exists
    */
    enum class VSync : uint8_t
    {
        Off = 0,     // immediate: no waiting, may tear
        On,          // fifo: waits for vertical blank, never tears
        Adaptive,    // fifo relaxed: like On, but a late frame is shown at once and may tear
        Fast,        // mailbox: no waiting and no tearing; the newest finished frame is shown at vertical blank
        LatestReady  // fifo latest ready: like On, but at vertical blank shows the newest finished frame and skips older ones
    };

    struct Version
    {
    public:
        uint32_t major = 0;
        uint32_t minor = 0;
        uint32_t patch = 0;

        constexpr Version() = default;

        constexpr Version(uint32_t major, uint32_t minor, uint32_t patch = 0) : major(major), minor(minor), patch(patch)
        {
        }
    };

    struct ContextPreferences
    {
        std::string applicationName{"TestApplication"};
        std::string engineName{"TestEngine"};
        Version applicationVersion{};
        Version engineVersion{};
        GpuPreference preferredGpu{GpuPreference::Discrete};
        VSync preferredVSync{VSync::Fast};

        std::vector<Feature> requiredFeatures;
        std::vector<Feature> optionalFeatures;

        bool syncValidation{false};
    };
} //namespace vela::core

#endif //VELA_CORE_CONTEXT_PREFERENCES_HPP