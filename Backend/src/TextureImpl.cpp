#include "TextureImpl.hpp"
#include "ContextImpl.hpp"
#include "Vela/Core/Context.hpp"

#include "stb_image.h"

#include <cstring>
#include <stdexcept>

namespace vela::backend
{
    TextureImpl::TextureImpl(core::Context& context, const std::string& path)
    {
        int width, height, channels;
        stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!pixels)
            throw std::runtime_error("Failed to load image: " + path);

        m_device = context.impl()->getDevice();
        m_allocator = context.impl()->getAllocator();

        VkDeviceSize imageSize = width * height * 4;

        VkBufferCreateInfo stagingCI{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        stagingCI.size = imageSize;
        stagingCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo stagingAllocCI{};
        stagingAllocCI.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                            | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuf;
        VmaAllocation stagingAlloc;
        VmaAllocationInfo stagingInfo{};
        vmaCreateBuffer(m_allocator, &stagingCI, &stagingAllocCI, &stagingBuf, &stagingAlloc, &stagingInfo);

        std::memcpy(stagingInfo.pMappedData, pixels, imageSize);
        stbi_image_free(pixels);

        VkImageCreateInfo imageCI{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        imageCI.mipLevels = 1;
        imageCI.arrayLayers = 1;
        imageCI.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL; 
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo imageAllocCI{};
        imageAllocCI.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateImage(m_allocator, &imageCI, &imageAllocCI, &m_image, &m_allocation, nullptr);

        VkCommandBuffer commandBuffer{};

        VkCommandBufferAllocateInfo commandBufferAllocatInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        commandBufferAllocatInfo.commandBufferCount = 1;
        commandBufferAllocatInfo.commandPool = context.impl()->getGraphicsCommandPool();
        commandBufferAllocatInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkAllocateCommandBuffers(m_device, &commandBufferAllocatInfo, &commandBuffer);

        VkCommandBufferBeginInfo commandBufferBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

        // For UNDEFINED → TRANSFER_DST_OPTIMAL:
        VkImageMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        barrier.image = m_image;
        barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;

        VkDependencyInfo dependencyInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;  
        region.bufferImageHeight = 0;
        region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.imageOffset = {0,0,0};
        region.imageExtent = imageCI.extent;

        vkCmdCopyBufferToImage(commandBuffer, stagingBuf, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // For TRANSFER_DST_OPTIMAL → SHADER_READ_ONLY_OPTIMAL:
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

        dependencyInfo.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

        vkEndCommandBuffer(commandBuffer);

        VkCommandBufferSubmitInfo commandBufferSubmitInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
        commandBufferSubmitInfo.commandBuffer = commandBuffer;

        VkSubmitInfo2 submit{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
        submit.pCommandBufferInfos = &commandBufferSubmitInfo;
        submit.commandBufferInfoCount = 1;

        // The graphics queue, not the transfer one: the barriers above are
        // recorded into a pool owned by the graphics family.
        VkQueue uploadQueue = context.impl()->getGraphicsQueue();

        vkQueueSubmit2(uploadQueue, 1, &submit, VK_NULL_HANDLE);

        vkQueueWaitIdle(uploadQueue);

        vkFreeCommandBuffers(m_device, context.impl()->getGraphicsCommandPool(), 1, &commandBuffer);

        vmaDestroyBuffer(m_allocator, stagingBuf, stagingAlloc);

        VkImageViewCreateInfo viewCI{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewCI.image = m_image;
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewCI.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        if(VkResult result = vkCreateImageView(m_device, &viewCI, nullptr, &m_imageView); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create image view");

        VkSamplerCreateInfo samplerCI{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerCI.magFilter = VK_FILTER_LINEAR;
        samplerCI.minFilter = VK_FILTER_LINEAR;
        samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCI.maxLod = 1.0f;

        if(VkResult result = vkCreateSampler(m_device, &samplerCI, nullptr, &m_sampler); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create sampler");
    }

    VkImageView TextureImpl::getImageView() const
    {
        return m_imageView;
    }

    VkSampler TextureImpl::getSampler() const
    {
        return m_sampler;
    }

    TextureImpl::~TextureImpl()
    {
        if (m_sampler)   
            vkDestroySampler(m_device, m_sampler, nullptr);

        if (m_imageView) 
            vkDestroyImageView(m_device, m_imageView, nullptr);

        if (m_image)     
            vmaDestroyImage(m_allocator, m_image, m_allocation);
    }
} //namespace vela::backend