#include "DescriptorPool.hpp"

#include <array>
#include <stdexcept>

namespace vela::backend
{
    DescriptorPool::DescriptorPool(VkDevice device) : m_device(device)
    {
    }

    VkDescriptorPool DescriptorPool::createPool()
    {
        const std::array<VkDescriptorPoolSize, 2> poolSizes
        {
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k_setsPerPool * 8},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, k_setsPerPool}
        };

        VkDescriptorPoolCreateInfo descriptorPoolCI{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        descriptorPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolCI.pPoolSizes = poolSizes.data();
        descriptorPoolCI.maxSets = k_setsPerPool;

        VkDescriptorPool pool{VK_NULL_HANDLE};

        if (vkCreateDescriptorPool(m_device, &descriptorPoolCI, nullptr, &pool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor pool");

        m_pools.push_back(pool);

        return pool;
    }

    DescriptorSetAllocation DescriptorPool::allocate(VkDescriptorSetLayout layout)
    {
        if (m_pools.empty())
            createPool();

        VkDescriptorSetAllocateInfo descriptorSetAI{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        descriptorSetAI.descriptorPool = m_pools.back();
        descriptorSetAI.descriptorSetCount = 1;
        descriptorSetAI.pSetLayouts = &layout;

        DescriptorSetAllocation allocation;
        allocation.pool = m_pools.back();

        VkResult result = vkAllocateDescriptorSets(m_device, &descriptorSetAI, &allocation.set);

        if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
        {
            allocation.pool = createPool();
            descriptorSetAI.descriptorPool = allocation.pool;

            result = vkAllocateDescriptorSets(m_device, &descriptorSetAI, &allocation.set);
        }

        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate descriptor set");

        return allocation;
    }

    void DescriptorPool::free(const DescriptorSetAllocation& allocation)
    {
        if (allocation.set == VK_NULL_HANDLE || allocation.pool == VK_NULL_HANDLE)
            return;

        vkFreeDescriptorSets(m_device, allocation.pool, 1, &allocation.set);
    }

    void DescriptorPool::clear()
    {
        for (VkDescriptorPool pool : m_pools)
            vkDestroyDescriptorPool(m_device, pool, nullptr);

        m_pools.clear();
    }

    DescriptorPool::~DescriptorPool()
    {
        clear();
    }
} //namespace vela::backend
