#include "Vela/Graphics/Mesh.hpp"
#include "MeshImpl.hpp"

namespace vela::graphics
{
    Mesh::Mesh(core::Context& ctx, std::span<const Vertex> vertices) : m_impl(std::make_unique<backend::MeshImpl>(ctx, vertices))
    {
        
    }

    Mesh::~Mesh() = default;

    Mesh::Mesh(Mesh&&) noexcept = default;

    Mesh& Mesh::operator=(Mesh&&) noexcept = default;

    backend::MeshImpl* Mesh::impl() const { return m_impl.get(); } 

} //namespace vela::graphics