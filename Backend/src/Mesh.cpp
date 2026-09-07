#include "Vela/Graphics/Mesh.hpp"

#include <stdexcept>
#include "MeshImpl.hpp"

namespace vela::graphics
{
    Result<Mesh> Mesh::create(core::Context& ctx, std::span<const std::byte> vertexData, uint32_t vertexCount, std::span<const uint32_t> indices)
    {
        try
        {
            return Mesh(ctx, vertexData, vertexCount, indices);
        }
        catch (const std::exception& error)
        {
            return Error{ErrorCode::AllocationFailed, error.what()};
        }
    }

    Mesh::Mesh(core::Context& ctx, std::span<const std::byte> vertexData, uint32_t vertexCount, std::span<const uint32_t> indices)
        : m_impl(std::make_unique<backend::MeshImpl>(ctx, vertexData, vertexCount, indices)) {}

    Mesh::~Mesh() = default;
    Mesh::Mesh(Mesh&&) noexcept = default;
    Mesh& Mesh::operator=(Mesh&&) noexcept = default;

    backend::MeshImpl* Mesh::impl() const { return m_impl.get(); }
} //namespace vela::graphics