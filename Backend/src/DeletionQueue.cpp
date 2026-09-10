#include "DeletionQueue.hpp"

namespace vela::backend
{
    DeletionQueue::DeletionQueue(VkDevice device, VmaAllocator allocator) :
    m_allocator(allocator), m_device(device) 
    {

    }

    void DeletionQueue::push(const DeletionEntry& entry)
    {
        m_pending.push_back({m_currentFrame, entry});
    }

    void DeletionQueue::retireFrame()
    {
        ++m_currentFrame;

        std::erase_if(m_pending, [this](const std::pair<uint64_t, DeletionEntry>& two)
        {
            if(two.first + k_framesInFlight <= m_currentFrame)
            {
                destroy(two.second);
                return true;
            }

            return false;
        });
    }

    void DeletionQueue::flushAll()
    {
        for(const auto& entry : m_pending)
            destroy(entry.second);
        
        m_pending.clear();
    }

    void DeletionQueue::destroy(const DeletionEntry& entry)
    {
        if(entry.sampler)
            vkDestroySampler(m_device, entry.sampler, nullptr);
        if(entry.imageView)
            vkDestroyImageView(m_device, entry.imageView, nullptr);
        if(entry.image)
            vmaDestroyImage(m_allocator, entry.image, entry.allocation);
        if(entry.buffer)
            vmaDestroyBuffer(m_allocator, entry.buffer, entry.allocation);
        if (entry.descriptorSet.set && entry.descriptorSet.pool)
            vkFreeDescriptorSets(m_device, entry.descriptorSet.pool, 1, &entry.descriptorSet.set);
    }
} //namespace vela::backend