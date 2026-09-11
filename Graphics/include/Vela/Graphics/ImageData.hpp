#ifndef VELA_GRAPHICS_IMAGE_DATA_HPP
#define VELA_GRAPHICS_IMAGE_DATA_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#include "RenderTypes.hpp"

namespace vela::graphics
{
    struct ImageData
    {
        std::span<const std::byte> pixels;
        uint32_t width{0};
        uint32_t height{0};
        TextureFormat format{TextureFormat::RGBA8Srgb};
        uint32_t levels{1};
    };

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
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_IMAGE_DATA_HPP