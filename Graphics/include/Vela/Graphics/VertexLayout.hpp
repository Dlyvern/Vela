#ifndef VELA_GRAPHICS_VERTEX_LAYOUT_HPP
#define VELA_GRAPHICS_VERTEX_LAYOUT_HPP

#include <cstdint>
#include <vector>

namespace vela::graphics
{
    enum class VertexAttributeFormat : uint8_t
    {
        Float = 0,
        Float2,
        Float3,
        Float4,
        Unorm8x4
    };

    enum class VertexInputRate : uint8_t 
    {
        Vertex = 0,
        Instance
    };

    struct VertexAttribute
    {
        uint32_t location{0};
        VertexAttributeFormat format{VertexAttributeFormat::Float};
        uint32_t offset{0};
    };

    struct VertexLayout
    {
        uint32_t stride{0};
        std::vector<VertexAttribute> attributes;
        VertexInputRate inputRate{VertexInputRate::Vertex};
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_VERTEX_LAYOUT_HPP
