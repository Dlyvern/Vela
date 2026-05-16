#ifndef VELA_BACKEND_TEXTURE_IMPL_HPP
#define VELA_BACKEND_TEXTURE_IMPL_HPP

#include <string>

#include "volk.h"
#include "vk_mem_alloc.h"

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::backend
{
    class TextureImpl
    {
    public:
        TextureImpl(core::Context& context, const std::string& path);
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

    };
} //namespace vela::backend

#endif //VELA_BACKEND_TEXTURE_IMPL_HPP