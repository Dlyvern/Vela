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
    class MeshImpl
    {
    public:
        MeshImpl(core::Context& ctx, std::span<const std::byte> vertexData, uint32_t vertexCount, std::span<const uint32_t> indices);
        ~MeshImpl();

        VkBuffer getVertexBuffer() const;
        VkBuffer getIndexBuffer() const;

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
    };
} //namespace vela::backend

#endif //VELA_BACKEND_MESH_IMPL_HPP
