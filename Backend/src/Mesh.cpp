#include "Vela/Graphics/Mesh.hpp"

#include <stdexcept>
#include "MeshImpl.hpp"

namespace vela::graphics
{
    Result<Mesh> Mesh::create(core::Context& ctx, std::span<const std::byte> vertexData, uint32_t vertexCount, std::span<const uint32_t> indices)
    {
        try
        {
            return Mesh(ctx, vertexData, vertexCount, std::as_bytes(indices),
                static_cast<uint32_t>(indices.size()), IndexType::Uint32);
        }
        catch (const std::exception& error)
        {
            return Error{ErrorCode::AllocationFailed, error.what()};
        }
    }

    Result<Mesh> Mesh::create(core::Context& ctx, std::span<const std::byte> vertexData, uint32_t vertexCount, std::span<const uint16_t> indices)
    {
        try
        {
            return Mesh(ctx, vertexData, vertexCount, std::as_bytes(indices),
                static_cast<uint32_t>(indices.size()), IndexType::Uint16);
        }
        catch (const std::exception& error)
        {
            return Error{ErrorCode::AllocationFailed, error.what()};
        }
    }

    Mesh::Mesh(core::Context& ctx, std::span<const std::byte> vertexData, uint32_t vertexCount,
        std::span<const std::byte> indexData, uint32_t indexCount, IndexType indexType)
        : m_impl(std::make_unique<backend::MeshImpl>(ctx, vertexData, vertexCount, indexData, indexCount,
            indexType == IndexType::Uint16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32)) {}

    Mesh::~Mesh() = default;
    Mesh::Mesh(Mesh&&) noexcept = default;
    Mesh& Mesh::operator=(Mesh&&) noexcept = default;

    backend::MeshImpl* Mesh::impl() const { return m_impl.get(); }
} //namespace vela::graphics
