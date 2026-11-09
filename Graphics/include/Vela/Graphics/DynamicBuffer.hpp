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
    /*
        A CPU-written buffer for data that changes every frame: vertices, indices or a storage buffer.
        It keeps one copy per frame in flight plus one, so update() never overwrites data the GPU is still
        reading. Each copy is separate: write the buffer every frame it is drawn with (before execute(), or
        from a pass's record(); either way the data lands in the frame being recorded next).
    */
    class DynamicBuffer
    {
    public:
        enum class Usage : uint8_t
        {
            Vertex,
            Index,
            Storage
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

        // Creates the buffer.
        // ctx:          Vela context that owns the GPU memory.
        // usage:        what the buffer is bound as.
        // initialBytes: starting size of each copy; update() grows it when needed.
        // indexType:    only for Usage::Index: the type of the indices. Ignored otherwise.
        // Returns:      the buffer, or an error if GPU allocation failed.
        static Result<DynamicBuffer> create(core::Context& ctx, Usage usage, size_t initialBytes, IndexType indexType = IndexType::Uint32);

        // Writes data at the start of the copy the next recorded frame reads, growing every copy if it is too small.
        // data:    bytes to write.
        // Returns: an error if growing failed.
        Status update(std::span<const std::byte> data);

        // Size in bytes of each copy.
        size_t getSize() const;

        backend::DynamicBufferImpl* impl() const;

        ~DynamicBuffer();
    private:
        DynamicBuffer(core::Context& ctx, Usage usage, size_t initialBytes, IndexType indexType);
        
        std::unique_ptr<backend::DynamicBufferImpl> m_impl{nullptr};
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_DYNAMIC_BUFFER_HPP