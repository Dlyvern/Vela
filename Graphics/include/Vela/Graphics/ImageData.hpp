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
} //namespace vela::graphics

#endif //VELA_GRAPHICS_IMAGE_DATA_HPP