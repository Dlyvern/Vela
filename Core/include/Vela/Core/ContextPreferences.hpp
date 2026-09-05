#ifndef VELA_CORE_CONTEXT_PREFERENCES_HPP
#define VELA_CORE_CONTEXT_PREFERENCES_HPP

#include <string>
#include <cstdint>

namespace vela::core
{
    enum class GpuPreference : uint8_t
    {
        Discrete = 0,
        Integrated
    };

    enum class VSync : uint8_t
    {
        Off = 0,
        On,
        Adaptive,
        Fast
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
    };
} //namespace vela::core

#endif //VELA_CORE_CONTEXT_PREFERENCES_HPP