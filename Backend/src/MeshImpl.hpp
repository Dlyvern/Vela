#ifndef VELA_BACKEND_MESH_IMPL_HPP
#define VELA_BACKEND_MESH_IMPL_HPP

#include "volk.h"
#include "vk_mem_alloc.h"

#include <cstddef>
#include <cstdint>
#include <span>

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::backend
{
    class DeletionQueue;

    class MeshImpl
    {
    public:
        MeshImpl(core::Context& ctx, std::span<const std::byte> vertexData, uint32_t vertexCount,
            std::span<const std::byte> indexData, uint32_t indexCount, VkIndexType indexType);
        ~MeshImpl();

        VkBuffer getVertexBuffer() const;
        VkBuffer getIndexBuffer() const;
        VkIndexType getIndexType() const;

        bool isIndexed() const;
        uint32_t getIndexCount() const;
        uint32_t getVertexCount() const;
    private:
        VmaAllocator m_allocator{VK_NULL_HANDLE};

        VkBuffer m_vertexBuffer{VK_NULL_HANDLE};
        VmaAllocation m_vertexBufferAllocation{VK_NULL_HANDLE};

        VkBuffer m_indexBuffer{VK_NULL_HANDLE};
        VmaAllocation m_indexBufferAllocation{VK_NULL_HANDLE};

        uint32_t m_vertexCount{0};
        uint32_t m_indexCount{0};
        VkIndexType m_indexType{VK_INDEX_TYPE_UINT32};

        DeletionQueue* m_deletionQueue{nullptr};
    };
} //namespace vela::backend

#endif //VELA_BACKEND_MESH_IMPL_HPP
