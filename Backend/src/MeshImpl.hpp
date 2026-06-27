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
        MeshImpl(core::Context& ctx, std::span<const std::byte> vertexData, uint32_t vertexCount);
        ~MeshImpl();

        VkBuffer getBuffer() const { return m_buffer; }
        uint32_t getVertexCount() const { return m_vertexCount; }
    private:
        VmaAllocator m_allocator{VK_NULL_HANDLE};
        VkBuffer m_buffer{VK_NULL_HANDLE};
        VmaAllocation m_allocation{VK_NULL_HANDLE};
        uint32_t m_vertexCount{0};
    };
} //namespace vela::backend

#endif //VELA_BACKEND_MESH_IMPL_HPP
