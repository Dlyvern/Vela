#include "Vela/Graphics/Mesh.hpp"

#include <stdexcept>
#include "MeshImpl.hpp"

namespace vela::graphics
{
    Result<Mesh> Mesh::create(core::Context& ctx, std::span<const std::byte> vertexData, uint32_t vertexCount)
    {
        try
        {
            return Mesh(ctx, vertexData, vertexCount);
        }
        catch (const std::exception& error)
        {
            return Error{ErrorCode::AllocationFailed, error.what()};
        }
    }

    Mesh::Mesh(core::Context& ctx, std::span<const std::byte> vertexData, uint32_t vertexCount)
        : m_impl(std::make_unique<backend::MeshImpl>(ctx, vertexData, vertexCount)) {}

    Mesh::~Mesh() = default;
    Mesh::Mesh(Mesh&&) noexcept = default;
    Mesh& Mesh::operator=(Mesh&&) noexcept = default;

    backend::MeshImpl* Mesh::impl() const { return m_impl.get(); }
} //namespace vela::graphics