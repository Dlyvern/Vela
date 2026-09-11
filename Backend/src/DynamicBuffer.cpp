#include "Vela/Graphics/DynamicBuffer.hpp"

#include "Vela/Core/Context.hpp"
#include "DynamicBufferImpl.hpp"

namespace vela::graphics
{
    Result<DynamicBuffer> DynamicBuffer::create(core::Context& ctx, Usage usage, size_t initialBytes, IndexType indexType)
    {
        try
        {   
            return DynamicBuffer(ctx, usage, initialBytes, indexType);
        } 
        catch (const std::exception& error) 
        {
            return Error{ErrorCode::AllocationFailed, error.what()};
        }
    }

    DynamicBuffer::DynamicBuffer(DynamicBuffer&&) noexcept = default;
    
    DynamicBuffer& DynamicBuffer::operator=(DynamicBuffer&&) noexcept = default;

    DynamicBuffer::~DynamicBuffer()
    {
        
    }

    backend::DynamicBufferImpl* DynamicBuffer::impl() const
    {
        return m_impl.get();
    }

    Status DynamicBuffer::update(std::span<const std::byte> data)
    {
        return m_impl->update(data);
    }

    DynamicBuffer::DynamicBuffer(core::Context& ctx, Usage usage, size_t initialBytes, IndexType indexType)
    {
        //FIXME dog shit
        backend::DynamicBufferImpl::Usage us;

        switch (usage)
        {
            case Usage::Index:
                us = backend::DynamicBufferImpl::Usage::Index;
                break;
            case Usage::Vertex:
                us = backend::DynamicBufferImpl::Usage::Vertex;
                break;
        }

        VkIndexType vkIndexType;

        switch (indexType)
        {
        case IndexType::Uint16:
            vkIndexType = VK_INDEX_TYPE_UINT16;
            break;
        case IndexType::Uint32:
            vkIndexType = VK_INDEX_TYPE_UINT32;
          break;
        }

        m_impl = std::make_unique<backend::DynamicBufferImpl>(ctx, us, initialBytes, vkIndexType);
    }
} //namespace vela::graphics