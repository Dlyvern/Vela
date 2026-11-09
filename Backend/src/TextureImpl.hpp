#ifndef VELA_BACKEND_TEXTURE_IMPL_HPP
#define VELA_BACKEND_TEXTURE_IMPL_HPP

#include "Vela/Graphics/ImageData.hpp"
#include "Vela/Graphics/Texture.hpp"
#include "Vela/Graphics/Sampler.hpp"
#include "Vela/Result.hpp"

#include "volk.h"
#include "vk_mem_alloc.h"

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::backend
{
    class DeletionQueue;
    class ContextImpl;

    class TextureImpl
    {
    public:
        TextureImpl(core::Context& context, const graphics::ImageData& image, const graphics::SamplerDescription& samplerDescription,
            const graphics::TextureOptions& options);
        ~TextureImpl();

        VkImageView getImageView() const;
        VkSampler getSampler() const;

        Status update(const graphics::ImageData& image);

    private:
        VkDevice m_device{VK_NULL_HANDLE};
        VmaAllocator m_allocator{VK_NULL_HANDLE};

        VkImage m_image{VK_NULL_HANDLE};
        VmaAllocation m_allocation{VK_NULL_HANDLE};
        VkImageView m_imageView{VK_NULL_HANDLE};
        VkSampler m_sampler{VK_NULL_HANDLE};

        DeletionQueue* m_deletionQueue{nullptr};
        ContextImpl* m_context{nullptr};

        uint32_t m_width{0};
        uint32_t m_height{0};
        graphics::TextureFormat m_format{graphics::TextureFormat::RGBA8Srgb};
        uint32_t m_levels{1};
        uint32_t m_inputLevels{1};
        bool m_generateMips{false};

        // Fills levels 1.. from level 0 with a chain of linear blits; leaves levels 0..n-2 in TRANSFER_SRC_OPTIMAL
        void recordMipChain(VkCommandBuffer commandBuffer);

        // Copies every level of image into the texture and leaves it in SHADER_READ_ONLY_OPTIMAL
        Status upload(const graphics::ImageData& image, VkImageLayout currentLayout);
    };
} //namespace vela::backend

#endif //VELA_BACKEND_TEXTURE_IMPL_HPP