#ifndef VELA_GRAPHICS_VERTEX_LAYOUT_HPP
#define VELA_GRAPHICS_VERTEX_LAYOUT_HPP

#include <cstdint>
#include <vector>

namespace vela::graphics
{
    enum class VertexAttributeFormat : uint8_t
    {
        eFLOAT = 0,
        eFLOAT2,
        eFLOAT3,
        eFLOAT4
    };

    struct VertexAttribute
    {
        uint32_t location{0};
        VertexAttributeFormat format{VertexAttributeFormat::eFLOAT};
        uint32_t offset{0};
    };

    struct VertexLayout
    {
        uint32_t stride{0};
        std::vector<VertexAttribute> attributes;
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_VERTEX_LAYOUT_HPP
