#ifndef VELA_BACKEND_TEXTURE_IMPL_HPP
#define VELA_BACKEND_TEXTURE_IMPL_HPP

#include "Vela/Graphics/ImageData.hpp"
#include "Vela/Graphics/Sampler.hpp"

#include "volk.h"
#include "vk_mem_alloc.h"

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::backend
{
    class DeletionQueue;

    class TextureImpl
    {
    public:
        TextureImpl(core::Context& context, const graphics::ImageData& image, const graphics::SamplerDescription& samplerDescription = {});
        ~TextureImpl();

        VkImageView getImageView() const;
        VkSampler getSampler() const;

    private:
        VkDevice m_device{VK_NULL_HANDLE};
        VmaAllocator m_allocator{VK_NULL_HANDLE};

        VkImage m_image{VK_NULL_HANDLE};
        VmaAllocation m_allocation{VK_NULL_HANDLE};
        VkImageView m_imageView{VK_NULL_HANDLE};
        VkSampler m_sampler{VK_NULL_HANDLE};

        DeletionQueue* m_deletionQueue{nullptr};
    };
} //namespace vela::backend

#endif //VELA_BACKEND_TEXTURE_IMPL_HPP