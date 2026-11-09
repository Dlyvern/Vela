#ifndef VELA_GRAPHICS_RENDER_TYPES_HPP
#define VELA_GRAPHICS_RENDER_TYPES_HPP

#include <cstddef>
#include <cstdint>

namespace vela::graphics
{
    enum class TextureFormat : uint8_t
    {
        RGBA8Srgb = 0,
        RGBA8Unorm,
        RGBA16Float,
        Depth32Float,
        BC7Srgb,
        BC7Unorm,
        R8Unorm,
        RG8Unorm,
        RGBA32Float,
        BC1Srgb,
        BC1Unorm,
        BC3Srgb,
        BC3Unorm,
        BC4Unorm,
        BC5Unorm
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
        None = 0,
        Never,
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
        TransferSource,

        /*
            Samples what the attachment held at the end of the previous frame, e.g. the history of a temporal
            filter. Creates no ordering edge, so the same pass may also write the attachment. Makes the attachment
            persistent (see AttachmentSlot::persistent) and double-buffered: this frame's writers render into one
            image while History readers sample the other, and the two swap after each frame. Single-sampled only
        */
        History
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

    [[nodiscard]] constexpr bool isBlockFormat(TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::BC1Srgb:
            case TextureFormat::BC1Unorm:
            case TextureFormat::BC3Srgb:
            case TextureFormat::BC3Unorm:
            case TextureFormat::BC4Unorm:
            case TextureFormat::BC5Unorm:
            case TextureFormat::BC7Srgb:
            case TextureFormat::BC7Unorm: return true;

            case TextureFormat::RGBA8Srgb:
            case TextureFormat::RGBA8Unorm:
            case TextureFormat::RGBA16Float:
            case TextureFormat::RGBA32Float:
            case TextureFormat::R8Unorm:
            case TextureFormat::RG8Unorm:
            case TextureFormat::Depth32Float: return false;
        }

        return false;
    }

    [[nodiscard]] constexpr uint32_t blockSizeBytes(TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::BC1Srgb:
            case TextureFormat::BC1Unorm:
            case TextureFormat::BC4Unorm: return 8;

            case TextureFormat::BC3Srgb:
            case TextureFormat::BC3Unorm:
            case TextureFormat::BC5Unorm:
            case TextureFormat::BC7Srgb:
            case TextureFormat::BC7Unorm: return 16;

            case TextureFormat::RGBA8Srgb:
            case TextureFormat::RGBA8Unorm:
            case TextureFormat::RGBA16Float:
            case TextureFormat::RGBA32Float:
            case TextureFormat::R8Unorm:
            case TextureFormat::RG8Unorm:
            case TextureFormat::Depth32Float: return 0;
        }

        return 0;
    }

    [[nodiscard]] constexpr uint32_t bytesPerPixel(TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::R8Unorm:      return 1;
            case TextureFormat::RG8Unorm:     return 2;
            case TextureFormat::RGBA8Srgb:
            case TextureFormat::RGBA8Unorm:   return 4;
            case TextureFormat::RGBA16Float:  return 8;
            case TextureFormat::RGBA32Float:  return 16;
            case TextureFormat::Depth32Float: return 4;
            case TextureFormat::BC1Srgb:
            case TextureFormat::BC1Unorm:
            case TextureFormat::BC3Srgb:
            case TextureFormat::BC3Unorm:
            case TextureFormat::BC4Unorm:
            case TextureFormat::BC5Unorm:
            case TextureFormat::BC7Srgb:
            case TextureFormat::BC7Unorm: return 0;
        }

        return 0;
    }

    [[nodiscard]] constexpr std::size_t imageSizeBytes(TextureFormat format, uint32_t width, uint32_t height)
    {
        if (isBlockFormat(format))
        {
            constexpr uint32_t blockWidth = 4;
            constexpr uint32_t blockHeight = 4;

            const uint32_t blocksX = (width + blockWidth - 1) / blockWidth;

            const uint32_t blocksY = (height + blockHeight - 1) / blockHeight;

            return static_cast<std::size_t>(blocksX) *
                static_cast<std::size_t>(blocksY) *
                blockSizeBytes(format);
        }

        return static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
            bytesPerPixel(format);
    }

    static_assert(bytesPerPixel(TextureFormat::RGBA8Srgb) == 4);
    static_assert(bytesPerPixel(TextureFormat::RGBA16Float) == 8);
    static_assert(blockSizeBytes(TextureFormat::BC7Srgb) == 16);
    static_assert(imageSizeBytes(TextureFormat::BC7Srgb, 1, 1) == 16);
    static_assert(imageSizeBytes(TextureFormat::BC7Srgb, 2048, 2048) == 4194304);
    static_assert(imageSizeBytes(TextureFormat::RGBA8Srgb, 16, 16) == 1024);
    static_assert(imageSizeBytes(TextureFormat::BC1Srgb, 4, 4) == 8);
    static_assert(imageSizeBytes(TextureFormat::RG8Unorm, 4, 4) == 32);

} //namespace vela::graphics

#endif //VELA_GRAPHICS_RENDER_TYPES_HPP