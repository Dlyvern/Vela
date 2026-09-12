#include "DynamicBufferImpl.hpp"

#include "ContextImpl.hpp"
#include "DeletionQueue.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>


namespace vela::backend
{
    DynamicBufferImpl::DynamicBufferImpl(core::Context& ctx, Usage usage, size_t initialBytes, VkIndexType indexType) : m_size(initialBytes), m_usage(usage),
    m_allocator(ctx.impl()->getAllocator()), m_deletionQueue(&ctx.impl()->getDeletionQueue()), m_indexType(indexType)
    {
        reallocate(m_size);
    }

    size_t DynamicBufferImpl::currentSlot() const
    {
        return m_deletionQueue->currentFrame() % k_slotCount;
    }

    size_t DynamicBufferImpl::getSize() const
    {
        return m_size;
    }

    bool DynamicBufferImpl::reallocate(size_t newSize)
    {
        if (newSize == 0)
            return true;

        VkBufferCreateInfo bufferCI{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};

        switch (m_usage)
        {
            case Usage::Index:   bufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT; break;
            case Usage::Vertex:  bufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; break;
            case Usage::Storage: bufferCI.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; break;
        }

        bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferCI.size = newSize;

        VmaAllocationCreateInfo allocationCI{};
        allocationCI.usage = VMA_MEMORY_USAGE_AUTO;
        allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                      | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        for (Slot& slot : m_slots)
        {
            VmaAllocationInfo allocationInfo{};

            VkBuffer buffer{VK_NULL_HANDLE};
            VmaAllocation allocation{VK_NULL_HANDLE};

            if(VkResult result = vmaCreateBuffer(m_allocator, &bufferCI, &allocationCI, &buffer, &allocation, &allocationInfo);
                result != VK_SUCCESS)
            {
                std::cerr << "Failed to create dynamic buffer\n";
                return false;
            }

            if (slot.buffer)
                m_deletionQueue->push({.buffer = slot.buffer, .allocation = slot.allocation});

            slot.buffer = buffer;
            slot.allocation = allocation;
            slot.mappedData = allocationInfo.pMappedData;
        }

        m_size = newSize;

        return true;
    }

    VkIndexType DynamicBufferImpl::getIndexType() const
    {
        return m_indexType;
    }

    VkBuffer DynamicBufferImpl::getBuffer() const
    {
        return m_slots[currentSlot()].buffer;
    }

    Status DynamicBufferImpl::update(std::span<const std::byte> data)
    {
        if (data.empty())
            return {};

        if (data.size() > m_size)
        {
            const size_t newSize = std::max<size_t>(m_size * 2, data.size());

            if(!reallocate(newSize))
                return Error{ErrorCode::AllocationFailed, "Failed to realocate buffer"};
        }

        const Slot& slot = m_slots[currentSlot()];

        std::memcpy(slot.mappedData, data.data(), data.size());
        vmaFlushAllocation(m_allocator, slot.allocation, 0, VK_WHOLE_SIZE);

        return {};
    }

    DynamicBufferImpl::~DynamicBufferImpl()
    {
        if (m_deletionQueue == nullptr)
            return;

        for (const Slot& slot : m_slots)
            if (slot.buffer)
                m_deletionQueue->push({.buffer = slot.buffer, .allocation = slot.allocation});
    }
} //namespace vela::backend
