#ifndef VELA_BUILTINS_SHAPES_HPP
#define VELA_BUILTINS_SHAPES_HPP

#include "Vela/Graphics/Vertex.hpp"

#include <cstdint>
#include <vector>

namespace vela::builtins
{
    template<typename V>
    struct MeshData
    {
        std::vector<V> vertices;
        std::vector<uint32_t> indices;
    };
} //namespace vela::builtins

namespace vela::builtins::shapes2d
{
    [[nodiscard]] MeshData<graphics::SpriteVertex> triangle();
    [[nodiscard]] MeshData<graphics::SpriteVertex> quad();
    [[nodiscard]] MeshData<graphics::SpriteVertex> circle(uint32_t segments = 32);
} //namespace vela::builtins::shapes2d

namespace vela::builtins::shapes3d
{
    [[nodiscard]] MeshData<graphics::StaticVertex> triangle();
    [[nodiscard]] MeshData<graphics::StaticVertex> plane();
    [[nodiscard]] MeshData<graphics::StaticVertex> cube();
    [[nodiscard]] MeshData<graphics::StaticVertex> sphere(uint32_t rings = 16, uint32_t segments = 32);
} //namespace vela::builtins::shapes3d

#endif //VELA_BUILTINS_SHAPES_HPP
