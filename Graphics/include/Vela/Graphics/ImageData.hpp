#ifndef VELA_GRAPHICS_IMAGE_DATA_HPP
#define VELA_GRAPHICS_IMAGE_DATA_HPP

#include <cstddef>
#include <cstdint>
#include <span>

namespace vela::graphics
{
    enum class TextureFormat : uint8_t
    {
        RGBA8Srgb = 0,
        RGBA8Unorm
    };

    [[nodiscard]] constexpr uint32_t bytesPerPixel(TextureFormat)
    {
        return 4;
    }

    struct ImageData
    {
        std::span<const std::byte> pixels;
        uint32_t width{0};
        uint32_t height{0};
        TextureFormat format{TextureFormat::RGBA8Srgb};
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_IMAGE_DATA_HPP
