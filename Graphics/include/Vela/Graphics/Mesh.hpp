#ifndef VELA_GRAPHICS_MESH_HPP
#define VELA_GRAPHICS_MESH_HPP

#include "Vertex.hpp"

#include <memory>
#include <span>

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
        Mesh(core::Context& ctx, std::span<const Vertex> vertices);
        ~Mesh();
        
        Mesh(Mesh&&) noexcept;
        Mesh& operator=(Mesh&&) noexcept;

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        backend::MeshImpl* impl() const;
    private:
        std::unique_ptr<backend::MeshImpl> m_impl{nullptr};
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_MESH_HPP