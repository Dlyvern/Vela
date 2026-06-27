#ifndef VELA_GRAPHICS_VERTEX_HPP
#define VELA_GRAPHICS_VERTEX_HPP

namespace vela::graphics
{
    struct SpriteVertex
    {
        float position[2];
        float uv[2];
    };

    struct StaticVertex
    {
        float position[3];
        float uv[2];
    };

    struct SkinnedVertex
    {
        float position[3];
        float uv[2];
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_VERTEX_HPP