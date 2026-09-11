#ifndef VELA_CORE_FEATURE_HPP
#define VELA_CORE_FEATURE_HPP

#include <cstdint>

namespace vela::core
{
    enum class Feature : uint8_t
    {
        SamplerAnisotropy = 0,
        FillModeNonSolid,
        WideLines,
        Bindless,
        RayTracing,
        MeshShaders
    };

    constexpr const char* toString(Feature feature)
    {
        switch (feature)
        {
            case Feature::SamplerAnisotropy: return "SamplerAnisotropy";
            case Feature::FillModeNonSolid:  return "FillModeNonSolid";
            case Feature::WideLines:         return "WideLines";
            case Feature::Bindless:          return "Bindless";
            case Feature::RayTracing:        return "RayTracing";
            case Feature::MeshShaders:       return "MeshShaders";
        }

        return "Unknown";
    }
} //namespace vela::core

#endif //VELA_CORE_FEATURE_HPP