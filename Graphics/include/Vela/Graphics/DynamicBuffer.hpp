#ifndef VELA_GRAPHICS_DYNAMIC_BUFFER_HPP
#define VELA_GRAPHICS_DYNAMIC_BUFFER_HPP

#include <cstdint>
#include <span>
#include <memory>

#include "Vela/Result.hpp"

namespace vela::backend
{
    class DynamicBufferImpl;
} //namespace vela::backend

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::graphics
{
    class DynamicBuffer
    {
    public:
        enum class Usage : uint8_t 
        {
            Vertex,
            Index
        };

        enum class IndexType
        {
            Uint16 = 0,
            Uint32,
        };

        DynamicBuffer(DynamicBuffer&&) noexcept;
        DynamicBuffer& operator=(DynamicBuffer&&) noexcept;

        DynamicBuffer(const DynamicBuffer&) = delete;
        DynamicBuffer& operator=(const DynamicBuffer&) = delete;

        static Result<DynamicBuffer> create(core::Context& ctx, Usage usage, size_t initialBytes, IndexType indexType = IndexType::Uint32);

        Status update(std::span<const std::byte> data);

        backend::DynamicBufferImpl* impl() const;

        ~DynamicBuffer();
    private:
        DynamicBuffer(core::Context& ctx, Usage usage, size_t initialBytes, IndexType indexType);
        
        std::unique_ptr<backend::DynamicBufferImpl> m_impl{nullptr};
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_DYNAMIC_BUFFER_HPP