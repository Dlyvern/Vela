#include "MeshImpl.hpp"
#include "ContextImpl.hpp"
#include "Vela/Core/Context.hpp"

#include <cstring>
#include <stdexcept>

namespace vela::backend
{
    MeshImpl::MeshImpl(core::Context& ctx, std::span<const std::byte> vertexData, uint32_t vertexCount, std::span<const uint32_t> indices)
    {
        for (uint32_t index : indices)
            if (index >= vertexCount)
                throw std::runtime_error("Mesh index " + std::to_string(index)
                    + " is out of range for " + std::to_string(vertexCount) + " vertices");

        m_allocator = ctx.impl()->getAllocator();
        m_vertexCount = vertexCount;
        m_indexCount = static_cast<uint32_t>(indices.size());

        VkBufferCreateInfo vertexBufferCI{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        vertexBufferCI.size = vertexData.size();
        vertexBufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        vertexBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocCI{};
        allocCI.usage = VMA_MEMORY_USAGE_AUTO;
        allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                      | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo vertexBufferInfo{};
        if (vmaCreateBuffer(m_allocator, &vertexBufferCI, &allocCI, &m_vertexBuffer, 
                &m_vertexBufferAllocation, &vertexBufferInfo) != VK_SUCCESS)
            throw std::runtime_error("Failed to create vertex buffer");

        std::memcpy(vertexBufferInfo.pMappedData, vertexData.data(), vertexData.size());
        vmaFlushAllocation(m_allocator, m_vertexBufferAllocation, 0, VK_WHOLE_SIZE);

        if(!indices.empty())
        {
            const VkDeviceSize indexBytes = indices.size_bytes();

            VkBufferCreateInfo indexBufferCI{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            indexBufferCI.size = indexBytes;
            indexBufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            indexBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationInfo indexBufferInfo{};

            if (vmaCreateBuffer(m_allocator, &indexBufferCI, &allocCI, &m_indexBuffer,
                    &m_indexBufferAllocation, &indexBufferInfo) != VK_SUCCESS)
                throw std::runtime_error("Failed to create index buffer");

            std::memcpy(indexBufferInfo.pMappedData, indices.data(), indexBytes);
            vmaFlushAllocation(m_allocator, m_indexBufferAllocation, 0, VK_WHOLE_SIZE);
        }
    }

    VkBuffer MeshImpl::getVertexBuffer() const 
    {
        return m_vertexBuffer;
    }

    VkBuffer MeshImpl::getIndexBuffer() const
    {
        return m_indexBuffer;
    }

    bool MeshImpl::isIndexed() const
    {
        return m_indexBuffer != VK_NULL_HANDLE;
    }

    uint32_t MeshImpl::getIndexCount() const
    {
        return m_indexCount;
    }

    uint32_t MeshImpl::getVertexCount() const
    {
        return m_vertexCount;
    }

    MeshImpl::~MeshImpl()
    {
        vmaDestroyBuffer(m_allocator, m_vertexBuffer, m_vertexBufferAllocation);

        if(m_indexBuffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(m_allocator, m_indexBuffer, m_indexBufferAllocation);
    }
} //namespace vela::backend
