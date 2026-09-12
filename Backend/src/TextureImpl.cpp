#include "TextureImpl.hpp"
#include "ContextImpl.hpp"
#include "Vela/Core/Context.hpp"

#include "Formats.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace vela::backend
{
    TextureImpl::TextureImpl(core::Context& context, const graphics::ImageData& image, const graphics::SamplerDescription& samplerDescription) : m_deletionQueue(&context.impl()->getDeletionQueue())
    {
        if (image.width == 0 || image.height == 0)
            throw std::runtime_error("Image extent must not be zero");

        if (graphics::isDepthFormat(image.format))
            throw std::runtime_error("Texture cannot be created from a depth format");

        if (image.levels == 0)
            throw std::runtime_error("Texture must have at least one mip level");

        size_t expected = 0;

        uint32_t mipWidth = image.width;
        uint32_t mipHeight = image.height;

        for (uint32_t level = 0; level < image.levels; ++level)
        {
            expected += graphics::imageSizeBytes(
                image.format,
                mipWidth,
                mipHeight
            );

            mipWidth = std::max(1u, mipWidth / 2);
            mipHeight = std::max(1u, mipHeight / 2);
        }

        if (image.pixels.size() < expected)
            throw std::runtime_error("Image pixel buffer is smaller than expected");

        const uint32_t width = image.width;
        const uint32_t height = image.height;
        const VkFormat format = toVkFormat(image.format);

        m_device = context.impl()->getDevice();
        m_allocator = context.impl()->getAllocator();

        VkDeviceSize imageSize = expected;

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

        std::memcpy(stagingInfo.pMappedData, image.pixels.data(), imageSize);
        vmaFlushAllocation(m_allocator, stagingAlloc, 0, VK_WHOLE_SIZE);

        VkImageCreateInfo imageCI{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.extent = {width, height, 1};
        imageCI.mipLevels = image.levels;
        imageCI.arrayLayers = 1;
        imageCI.format = format;
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
        barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, image.levels, 0, 1 };
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

        std::vector<VkBufferImageCopy> regions;
        regions.reserve(image.levels);

        VkDeviceSize bufferOffset = 0;

        mipWidth = width;
        mipHeight = height;

        for (uint32_t mip = 0; mip < image.levels; ++mip)
        {
            VkBufferImageCopy region{};

            region.bufferOffset = bufferOffset;

            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource = {
                VK_IMAGE_ASPECT_COLOR_BIT,
                mip,
                0,
                1
            };

            region.imageOffset = { 0, 0, 0 };

            region.imageExtent = {
                mipWidth,
                mipHeight,
                1
            };

            regions.push_back(region);

            bufferOffset += graphics::imageSizeBytes(
                image.format,
                mipWidth,
                mipHeight
            );

            mipWidth = std::max(1u, mipWidth / 2);
            mipHeight = std::max(1u, mipHeight / 2);
        }

        vkCmdCopyBufferToImage(commandBuffer, stagingBuf,
            m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<uint32_t>(regions.size()), regions.data());

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
        viewCI.format = format;
        viewCI.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, image.levels, 0, 1 };

        if(VkResult result = vkCreateImageView(m_device, &viewCI, nullptr, &m_imageView); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create image view");

        m_sampler = context.impl()->getSamplerCache().getSampler(samplerDescription);
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
        if(!m_deletionQueue)
            return;

        m_deletionQueue->push({.image = m_image, .imageView = m_imageView, .allocation = m_allocation});
    }
} //namespace vela::backend