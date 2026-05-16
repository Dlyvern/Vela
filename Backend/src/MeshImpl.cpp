#include "MeshImpl.hpp"
#include "ContextImpl.hpp"
#include "Vela/Core/Context.hpp"
#include <cstring>
#include <stdexcept>

namespace vela::backend
{
    MeshImpl::MeshImpl(core::Context& ctx, std::span<const graphics::Vertex> vertices)
    {
        auto contextImpl = ctx.impl();
        m_allocator = contextImpl->getAllocator();
        m_vertexCount = static_cast<uint32_t>(vertices.size());

        VkBufferCreateInfo bufferCI{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferCI.size = vertices.size() * sizeof(graphics::Vertex);
        bufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocCI{};
        allocCI.usage = VMA_MEMORY_USAGE_AUTO;
        allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                      | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo info{};
        if (vmaCreateBuffer(m_allocator, &bufferCI, &allocCI, &m_buffer, &m_allocation, &info) != VK_SUCCESS)
            throw std::runtime_error("Failed to create vertex buffer");

        std::memcpy(info.pMappedData, vertices.data(), bufferCI.size);
    }

    MeshImpl::~MeshImpl()
    {
        vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
    }
} //namespace vela::backend