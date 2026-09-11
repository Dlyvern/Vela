#ifndef VELA_BACKEND_DYNAMIC_BUFFER_IMPL_HPP
#define VELA_BACKEND_DYNAMIC_BUFFER_IMPL_HPP

#include "Vela/Core/Context.hpp"

#include "FrameConstants.hpp"

#include "volk.h"
#include "vk_mem_alloc.h"

#include <array>

namespace vela::backend
{
    class DeletionQueue;

    class DynamicBufferImpl
    {
    public:
        enum class Usage : uint8_t
        {
            Vertex,
            Index,
            Storage
        };

        DynamicBufferImpl(core::Context& ctx, Usage usage, size_t initialBytes, VkIndexType indexType = VK_INDEX_TYPE_UINT32);

        Status update(std::span<const std::byte> data);

        VkBuffer getBuffer() const;

        VkIndexType getIndexType() const;

        ~DynamicBufferImpl();

    private:
        static constexpr uint32_t k_slotCount = FrameConstants::k_framesInFlight;

        struct Slot
        {
            VkBuffer buffer{VK_NULL_HANDLE};
            VmaAllocation allocation{VK_NULL_HANDLE};
            void* mappedData{nullptr};
        };

        bool reallocate(size_t newSize);
        size_t currentSlot() const;

        std::array<Slot, k_slotCount> m_slots{};

        VmaAllocator m_allocator{VK_NULL_HANDLE};
        DeletionQueue* m_deletionQueue{nullptr};
        VkIndexType m_indexType{VK_INDEX_TYPE_UINT32};

        Usage m_usage;

        size_t m_size{0};
    };
} //namespace vela::backend

#endif //VELA_BACKEND_DYNAMIC_BUFFER_IMPL_HPP