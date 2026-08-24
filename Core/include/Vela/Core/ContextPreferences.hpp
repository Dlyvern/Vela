#ifndef VELA_CORE_CONTEXT_PREFERENCES_HPP
#define VELA_CORE_CONTEXT_PREFERENCES_HPP

#include <string>
#include <cstdint>

namespace vela::core
{
    enum class GPUDeviceType
    {
        eDISCRETE = 0,
        eINTEGRATED = 1
    };

    enum class PresentMode : uint8_t
    {
        eIMMEDIATE = 0,
        eMAILBOX,
        eFIFO,
        eFIFO_RELAXED
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
        GPUDeviceType preferedGpuType{GPUDeviceType::eDISCRETE};
        PresentMode preferedPresentMode{PresentMode::eMAILBOX};
    };
} //namespace vela::core

#endif //VELA_CORE_CONTEXT_PREFERENCES_HPP