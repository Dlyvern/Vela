#ifndef VELA_GRAPHICS_VERTEX_HPP
#define VELA_GRAPHICS_VERTEX_HPP

#include "Vela/Graphics/VertexLayout.hpp"

#include <cstddef>

namespace vela::graphics
{
    struct SpriteVertex
    {
        float position[2];
        float uv[2];

        static VertexLayout layout()
        {
            return VertexLayout
            {
                sizeof(SpriteVertex),
                {
                    {0, VertexAttributeFormat::Float2, offsetof(SpriteVertex, position)},
                    {1, VertexAttributeFormat::Float2, offsetof(SpriteVertex, uv)}
                }
            };
        }
    };

    struct StaticVertex
    {
        float position[3];
        float uv[2];
        float normal[3];

        static VertexLayout layout()
        {
            return VertexLayout
            {
                sizeof(StaticVertex),
                {
                    {0, VertexAttributeFormat::Float3, offsetof(StaticVertex, position)},
                    {1, VertexAttributeFormat::Float2, offsetof(StaticVertex, uv)},
                    {2, VertexAttributeFormat::Float3, offsetof(StaticVertex, normal)}
                }
            };
        }
    };

    struct SkinnedVertex
    {
        float position[3];
        float uv[2];
        float normal[3];

        static VertexLayout layout()
        {
            return VertexLayout
            {
                sizeof(SkinnedVertex),
                {
                    {0, VertexAttributeFormat::Float3, offsetof(SkinnedVertex, position)},
                    {1, VertexAttributeFormat::Float2, offsetof(SkinnedVertex, uv)},
                    {2, VertexAttributeFormat::Float3, offsetof(SkinnedVertex, normal)}
                }
            };
        }
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_VERTEX_HPP
