#ifndef VELA_GRAPHICS_RENDER_TYPES_HPP
#define VELA_GRAPHICS_RENDER_TYPES_HPP

#include <cstdint>

namespace vela::graphics
{
    enum class TextureFormat : uint8_t
    {
        RGBA8Srgb = 0,
        RGBA8Unorm,
        RGBA16Float,
        Depth32Float
    };

    enum class LoadOp : uint8_t  
    {
        Clear = 0,
        Load,
        DontCare
    };

    enum class StoreOp : uint8_t
    {
        Store = 0,
        DontCare
    };

    enum class CullMode : uint8_t
    {
        None = 0,
        Back,
        Front
    };

    enum class FrontFace : uint8_t 
    {
        Clockwise = 0,
        CounterClockwise
    };

    enum class DepthCompare : uint8_t
    {
        Never = 0,
        Less,
        Equal,
        LessOrEqual,
        Greater, 
        NotEqual,
        GreaterOrEqual,
        Always
    };

    enum class BlendMode : uint8_t
    {
        Opaque = 0,
        Alpha,
        Additive
    };

    enum class InputUsage : uint8_t
    {
        Sampled = 0,
        TransferSource
    };

    struct ClearValue
    {
        float r{0.0f};
        float g{0.0f};
        float b{0.0f};
        float a{1.0f};
        float depth{1.0f};
        uint32_t stencil{0};
    };

    struct Extent2D
    {
        uint32_t width{0};
        uint32_t height{0};
    };

    [[nodiscard]] constexpr bool isDepthFormat(TextureFormat format)
    {
        return format == TextureFormat::Depth32Float;
    }

    [[nodiscard]] constexpr uint32_t bytesPerPixel(TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::RGBA8Srgb:
            case TextureFormat::RGBA8Unorm:   return 4;
            case TextureFormat::RGBA16Float:  return 8;
            case TextureFormat::Depth32Float: return 4;
        }

        return 0;
    }

    static_assert(bytesPerPixel(TextureFormat::RGBA8Srgb) == 4);
    static_assert(bytesPerPixel(TextureFormat::RGBA16Float) == 8);

} //namespace vela::graphics

#endif //VELA_GRAPHICS_RENDER_TYPES_HPP