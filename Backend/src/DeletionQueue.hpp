#ifndef VELA_BACKEND_DELETION_QUEUE_HPP
#define VELA_BACKEND_DELETION_QUEUE_HPP

#include "volk.h"
#include "vk_mem_alloc.h"

#include "DescriptorPool.hpp"
#include "FrameConstants.hpp"

namespace vela::backend
{
    struct DeletionEntry
    {
        VkBuffer buffer{VK_NULL_HANDLE};
        VkImage image{VK_NULL_HANDLE};
        VkImageView imageView{VK_NULL_HANDLE};
        VkSampler sampler{VK_NULL_HANDLE};
        VmaAllocation allocation{VK_NULL_HANDLE};
        DescriptorSetAllocation descriptorSet{};
    };

    class DeletionQueue
    {
    public:
        DeletionQueue(VkDevice device, VmaAllocator allocator);

        DeletionQueue(const DeletionQueue&) = delete;
        DeletionQueue& operator=(const DeletionQueue&) = delete;

        void push(const DeletionEntry& entry);
        void retireFrame();
        void flushAll();

        [[nodiscard]] uint64_t currentFrame() const;

    private:
        void destroy(const DeletionEntry& entry);

        VmaAllocator m_allocator{VK_NULL_HANDLE};
        VkDevice m_device{VK_NULL_HANDLE};
        std::vector<std::pair<uint64_t, DeletionEntry>> m_pending;
        uint64_t m_currentFrame{0};
        static constexpr uint32_t k_framesInFlight = FrameConstants::k_framesInFlight;
    };
} //namespace vela::backend

#endif //VELA_BACKEND_DELETION_QUEUE_HPP