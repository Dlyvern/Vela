#ifndef VELA_GRAPHICS_SAMPLER_HPP
#define VELA_GRAPHICS_SAMPLER_HPP

#include <cstdint>

#include "RenderTypes.hpp"

namespace vela::graphics
{
    enum class Filter : uint8_t
    {
        Nearest,
        Linear
    };

    enum class AddressMode : uint8_t
    {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder
    };

    enum class MipMode : uint8_t
    {
        Nearest,
        Linear
    };

    struct SamplerDescription
    {
        Filter minFilter = Filter::Linear;
        Filter magFilter = Filter::Linear;

        AddressMode addressU = AddressMode::Repeat;
        AddressMode addressV = AddressMode::Repeat;
        AddressMode addressW = AddressMode::Repeat;

        MipMode mipMode = MipMode::Linear;

        float minLod = 0.0f;
        float maxLod = 1000.0f;

        float anisotropy = 1.0f;

        DepthCompare compareOp{DepthCompare::None};
    };
}

#endif //VELA_GRAPHICS_SAMPLER_HPP