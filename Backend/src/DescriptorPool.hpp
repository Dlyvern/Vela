#ifndef VELA_BACKEND_DESCRIPTOR_POOL_HPP
#define VELA_BACKEND_DESCRIPTOR_POOL_HPP

#include "volk.h"

#include <vector>

namespace vela::backend
{
    struct DescriptorSetAllocation
    {
        VkDescriptorSet set{VK_NULL_HANDLE};
        VkDescriptorPool pool{VK_NULL_HANDLE};
    };

    class DescriptorPool
    {
    public:
        DescriptorPool(VkDevice device);
        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool&) = delete;

        DescriptorSetAllocation allocate(VkDescriptorSetLayout layout);
        void free(const DescriptorSetAllocation& allocation);

        void clear();

        ~DescriptorPool();

    private:
        VkDescriptorPool createPool();

        static constexpr uint32_t k_setsPerPool{64};

        VkDevice m_device{VK_NULL_HANDLE};
        std::vector<VkDescriptorPool> m_pools;
    };
} //namespace vela::backend

#endif //VELA_BACKEND_DESCRIPTOR_POOL_HPP
