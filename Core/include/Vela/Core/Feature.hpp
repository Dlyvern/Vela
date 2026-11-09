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
        MeshShaders,

        // BC1-BC7 block-compressed textures (TextureFormat::BC*). Enabled automatically whenever the device
        // supports it, whether requested or not; without it Texture::create rejects BC formats
        TextureCompressionBC
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
            case Feature::TextureCompressionBC: return "TextureCompressionBC";
        }

        return "Unknown";
    }
} //namespace vela::core

#endif //VELA_CORE_FEATURE_HPP