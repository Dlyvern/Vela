#ifndef VELA_GRAPHICS_VERTEX_LAYOUT_HPP
#define VELA_GRAPHICS_VERTEX_LAYOUT_HPP

#include <cstdint>
#include <vector>

namespace vela::graphics
{
    // Format of one vertex attribute in the vertex buffer.
    // Float..Float4: 1-4 32-bit floats; the shader reads float/vec2/vec3/vec4.
    // Unorm8x4:      4 bytes mapped to 0..1; the shader reads vec4 (colours, compact weights).
    // Uint8x4:       4 unsigned bytes; the shader reads uvec4 (joint indices below 256).
    // Uint16x4:      4 unsigned 16-bit integers; the shader reads uvec4 (joint indices).
    enum class VertexAttributeFormat : uint8_t
    {
        Float = 0,
        Float2,
        Float3,
        Float4,
        Unorm8x4,
        Uint8x4,
        Uint16x4
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
