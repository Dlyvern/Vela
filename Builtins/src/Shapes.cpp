#include "Vela/Builtins/Shapes.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float k_pi = 3.14159265358979323846f;
    constexpr float k_extent = 0.5f;
} //namespace

namespace vela::builtins::shapes2d
{
    MeshData<graphics::SpriteVertex> triangle()
    {
        return
        {
            {
                {{ 0.0f, -k_extent}, {0.5f, 0.0f}},
                {{ k_extent,  k_extent}, {1.0f, 1.0f}},
                {{-k_extent,  k_extent}, {0.0f, 1.0f}},
            },
            { 0, 1, 2 }
        };
    }

    MeshData<graphics::SpriteVertex> quad()
    {
        return
        {
            {
                {{-k_extent, -k_extent}, {0.0f, 0.0f}},
                {{ k_extent, -k_extent}, {1.0f, 0.0f}},
                {{ k_extent,  k_extent}, {1.0f, 1.0f}},
                {{-k_extent,  k_extent}, {0.0f, 1.0f}},
            },
            { 0, 1, 2,  2, 3, 0 }
        };
    }

    MeshData<graphics::SpriteVertex> circle(uint32_t segments)
    {
        segments = std::max(segments, 3u);

        MeshData<graphics::SpriteVertex> data;
        data.vertices.reserve(segments + 1);
        data.indices.reserve(segments * 3);

        data.vertices.push_back({{0.0f, 0.0f}, {0.5f, 0.5f}});

        for (uint32_t segment = 0; segment < segments; ++segment)
        {
            const float angle = 2.0f * k_pi * static_cast<float>(segment) / static_cast<float>(segments);
            const float x = k_extent * std::cos(angle);
            const float y = k_extent * std::sin(angle);

            data.vertices.push_back({{x, y}, {x + 0.5f, y + 0.5f}});
        }

        for (uint32_t segment = 0; segment < segments; ++segment)
        {
            data.indices.push_back(0);
            data.indices.push_back(1 + segment);
            data.indices.push_back(1 + (segment + 1) % segments);
        }

        return data;
    }
} //namespace vela::builtins::shapes2d

namespace vela::builtins::shapes3d
{
    MeshData<graphics::StaticVertex> triangle()
    {
        return
        {
            {
                {{ 0.0f, -k_extent, 0.0f}, {0.5f, 0.0f}},
                {{ k_extent,  k_extent, 0.0f}, {1.0f, 1.0f}},
                {{-k_extent,  k_extent, 0.0f}, {0.0f, 1.0f}},
            },
            { 0, 1, 2 }
        };
    }

    MeshData<graphics::StaticVertex> plane()
    {
        return
        {
            {
                {{-k_extent, 0.0f,  k_extent}, {0.0f, 0.0f}},
                {{ k_extent, 0.0f,  k_extent}, {1.0f, 0.0f}},
                {{ k_extent, 0.0f, -k_extent}, {1.0f, 1.0f}},
                {{-k_extent, 0.0f, -k_extent}, {0.0f, 1.0f}},
            },
            { 0, 1, 2,  2, 3, 0 }
        };
    }

    MeshData<graphics::StaticVertex> cube()
    {
        return
        {
            {
                {{-k_extent, -k_extent,  k_extent}, {0.0f, 0.0f}},
                {{ k_extent, -k_extent,  k_extent}, {1.0f, 0.0f}},
                {{ k_extent,  k_extent,  k_extent}, {1.0f, 1.0f}},
                {{-k_extent,  k_extent,  k_extent}, {0.0f, 1.0f}},

                {{ k_extent, -k_extent, -k_extent}, {0.0f, 0.0f}},
                {{-k_extent, -k_extent, -k_extent}, {1.0f, 0.0f}},
                {{-k_extent,  k_extent, -k_extent}, {1.0f, 1.0f}},
                {{ k_extent,  k_extent, -k_extent}, {0.0f, 1.0f}},

                {{-k_extent, -k_extent, -k_extent}, {0.0f, 0.0f}},
                {{-k_extent, -k_extent,  k_extent}, {1.0f, 0.0f}},
                {{-k_extent,  k_extent,  k_extent}, {1.0f, 1.0f}},
                {{-k_extent,  k_extent, -k_extent}, {0.0f, 1.0f}},

                {{ k_extent, -k_extent,  k_extent}, {0.0f, 0.0f}},
                {{ k_extent, -k_extent, -k_extent}, {1.0f, 0.0f}},
                {{ k_extent,  k_extent, -k_extent}, {1.0f, 1.0f}},
                {{ k_extent,  k_extent,  k_extent}, {0.0f, 1.0f}},

                {{-k_extent,  k_extent,  k_extent}, {0.0f, 0.0f}},
                {{ k_extent,  k_extent,  k_extent}, {1.0f, 0.0f}},
                {{ k_extent,  k_extent, -k_extent}, {1.0f, 1.0f}},
                {{-k_extent,  k_extent, -k_extent}, {0.0f, 1.0f}},

                {{-k_extent, -k_extent, -k_extent}, {0.0f, 0.0f}},
                {{ k_extent, -k_extent, -k_extent}, {1.0f, 0.0f}},
                {{ k_extent, -k_extent,  k_extent}, {1.0f, 1.0f}},
                {{-k_extent, -k_extent,  k_extent}, {0.0f, 1.0f}},
            },
            {
                 0,  1,  2,   2,  3,  0,
                 4,  5,  6,   6,  7,  4,
                 8,  9, 10,  10, 11,  8,
                12, 13, 14,  14, 15, 12,
                16, 17, 18,  18, 19, 16,
                20, 21, 22,  22, 23, 20,
            }
        };
    }

    MeshData<graphics::StaticVertex> sphere(uint32_t rings, uint32_t segments)
    {
        rings = std::max(rings, 2u);
        segments = std::max(segments, 3u);

        const uint32_t columns = segments + 1;

        MeshData<graphics::StaticVertex> data;
        data.vertices.reserve((rings + 1) * columns);
        data.indices.reserve(rings * segments * 6);

        for (uint32_t ring = 0; ring <= rings; ++ring)
        {
            const float v = static_cast<float>(ring) / static_cast<float>(rings);
            const float phi = v * k_pi;
            const float sinPhi = std::sin(phi);
            const float cosPhi = std::cos(phi);

            for (uint32_t column = 0; column < columns; ++column)
            {
                const float u = static_cast<float>(column) / static_cast<float>(segments);
                const float theta = u * 2.0f * k_pi;

                data.vertices.push_back({
                    {k_extent * sinPhi * std::cos(theta),
                     k_extent * cosPhi,
                     k_extent * sinPhi * std::sin(theta)},
                    {u, v}
                });
            }
        }

        for (uint32_t ring = 0; ring < rings; ++ring)
        {
            for (uint32_t segment = 0; segment < segments; ++segment)
            {
                const uint32_t current = ring * columns + segment;
                const uint32_t next = current + columns;

                data.indices.push_back(current);
                data.indices.push_back(next);
                data.indices.push_back(current + 1);

                data.indices.push_back(current + 1);
                data.indices.push_back(next);
                data.indices.push_back(next + 1);
            }
        }

        return data;
    }
} //namespace vela::builtins::shapes3d
