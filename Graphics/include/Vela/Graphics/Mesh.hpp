#ifndef VELA_GRAPHICS_MESH_HPP
#define VELA_GRAPHICS_MESH_HPP

#include "Vertex.hpp"

#include "Vela/Result.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::backend
{
    class MeshImpl;
} //namespace vela::backend

namespace vela::graphics
{
    class Mesh
    {
    public:
        static Result<Mesh> create(core::Context& ctx, std::span<const std::byte> vertexData, uint32_t vertexCount,
        std::span<const uint32_t> indices = {});

        template<typename V>
        static Result<Mesh> create(core::Context& ctx, const std::vector<V>& verts,
            const std::vector<uint32_t>& indices = {})
        {
            return create(ctx, std::as_bytes(std::span<const V>(verts.data(), verts.size())),
                static_cast<uint32_t>(verts.size()), indices);
        }

        ~Mesh();

        Mesh(Mesh&&) noexcept;
        Mesh& operator=(Mesh&&) noexcept;

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        backend::MeshImpl* impl() const;
    private:
        Mesh(core::Context& ctx, std::span<const std::byte> vertexData, uint32_t vertexCount, std::span<const uint32_t> indices);

        std::unique_ptr<backend::MeshImpl> m_impl{nullptr};
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_MESH_HPP
