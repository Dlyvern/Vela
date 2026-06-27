#include "Vela/Graphics/Mesh.hpp"
#include "MeshImpl.hpp"

namespace vela::graphics
{
    Mesh::Mesh(core::Context& ctx, std::span<const std::byte> vertexData, uint32_t vertexCount)
        : m_impl(std::make_unique<backend::MeshImpl>(ctx, vertexData, vertexCount)) {}

    Mesh::~Mesh() = default;
    Mesh::Mesh(Mesh&&) noexcept = default;
    Mesh& Mesh::operator=(Mesh&&) noexcept = default;

    backend::MeshImpl* Mesh::impl() const { return m_impl.get(); }
} //namespace vela::graphics