#include "TextureImpl.hpp"
#include "ContextImpl.hpp"
#include "Vela/Core/Context.hpp"

#include "Formats.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace vela::backend
{
    namespace
    {
        // Bytes of every mip level together, or 0 if the image description is unusable
        size_t expectedSize(const graphics::ImageData& image)
        {
            size_t expected = 0;

            uint32_t mipWidth = image.width;
            uint32_t mipHeight = image.height;

            for (uint32_t level = 0; level < image.levels; ++level)
            {
                expected += graphics::imageSizeBytes(image.format, mipWidth, mipHeight);

                mipWidth = std::max(1u, mipWidth / 2);
                mipHeight = std::max(1u, mipHeight / 2);
            }

            return expected;
        }
    } //namespace

    TextureImpl::TextureImpl(core::Context& context, const graphics::ImageData& image,
        const graphics::SamplerDescription& samplerDescription, const graphics::TextureOptions& options) :
        m_deletionQueue(&context.impl()->getDeletionQueue()), m_context(context.impl()), m_generateMips(options.generateMips)
    {
        if (image.width == 0 || image.height == 0)
            throw std::runtime_error("Image extent must not be zero");

        if (graphics::isDepthFormat(image.format))
            throw std::runtime_error("Texture cannot be created from a depth format");

        if (image.levels == 0)
            throw std::runtime_error("Texture must have at least one mip level");

        if (graphics::isBlockFormat(image.format) && !context.impl()->getDeviceInfo().enabled(core::Feature::TextureCompressionBC))
            throw std::runtime_error("This device does not support BC-compressed textures (Feature::TextureCompressionBC); "
                "upload an uncompressed format instead");

        if (image.pixels.size() < expectedSize(image))
            throw std::runtime_error("Image pixel buffer is smaller than expected");

        if (m_generateMips)
        {
            if (image.levels != 1)
                throw std::runtime_error("TextureOptions::generateMips needs an image with exactly one level");

            VkFormatProperties formatProperties{};
            vkGetPhysicalDeviceFormatProperties(context.impl()->getPhysicalDevice(), toVkFormat(image.format), &formatProperties);

            constexpr VkFormatFeatureFlags k_mipFeatures = VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT
                | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

            if (graphics::isBlockFormat(image.format) || (formatProperties.optimalTilingFeatures & k_mipFeatures) != k_mipFeatures)
                throw std::runtime_error("TextureOptions::generateMips: the device can't filter-blit this format "
                    "(block-compressed formats never can); generate the mips on the CPU instead");
        }

        const uint32_t width = image.width;
        const uint32_t height = image.height;
        const VkFormat format = toVkFormat(image.format);

        m_device = context.impl()->getDevice();
        m_allocator = context.impl()->getAllocator();
        m_width = width;
        m_height = height;
        m_format = image.format;
        m_inputLevels = image.levels;
        m_levels = m_generateMips
            ? static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(std::max(width, height))))) + 1
            : image.levels;

        VkImageCreateInfo imageCI{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.extent = {width, height, 1};
        imageCI.mipLevels = m_levels;
        imageCI.arrayLayers = 1;
        imageCI.format = format;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL; 
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            | (m_generateMips ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0u);
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo imageAllocCI{};
        imageAllocCI.usage = VMA_MEMORY_USAGE_AUTO;

        if (vmaCreateImage(m_allocator, &imageCI, &imageAllocCI, &m_image, &m_allocation, nullptr) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate texture image");

        if (auto uploaded = upload(image, VK_IMAGE_LAYOUT_UNDEFINED); !uploaded)
            throw std::runtime_error(uploaded.error().message);

        VkImageViewCreateInfo viewCI{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewCI.image = m_image;
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format = format;
        viewCI.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, m_levels, 0, 1 };

        if(VkResult result = vkCreateImageView(m_device, &viewCI, nullptr, &m_imageView); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create image view");

        m_sampler = context.impl()->getSamplerCache().getSampler(samplerDescription);
    }

    Status TextureImpl::update(const graphics::ImageData& image)
    {
        if (image.width != m_width || image.height != m_height || image.format != m_format || image.levels != m_inputLevels)
            return Error{ErrorCode::InvalidArgument, "Texture::update needs the size, format and level count the texture was created with"};

        if (image.pixels.size() < expectedSize(image))
            return Error{ErrorCode::InvalidArgument, "Image pixel buffer is smaller than expected"};

        return upload(image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    Status TextureImpl::upload(const graphics::ImageData& image, VkImageLayout currentLayout)
    {
        const VkDeviceSize imageSize = expectedSize(image);

        VkBufferCreateInfo stagingCI{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        stagingCI.size = imageSize;
        stagingCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo stagingAllocCI{};
        stagingAllocCI.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                            | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuf{VK_NULL_HANDLE};
        VmaAllocation stagingAlloc{VK_NULL_HANDLE};
        VmaAllocationInfo stagingInfo{};

        if (vmaCreateBuffer(m_allocator, &stagingCI, &stagingAllocCI, &stagingBuf, &stagingAlloc, &stagingInfo) != VK_SUCCESS)
            return Error{ErrorCode::AllocationFailed, "Failed to allocate texture staging buffer"};

        std::memcpy(stagingInfo.pMappedData, image.pixels.data(), imageSize);
        vmaFlushAllocation(m_allocator, stagingAlloc, 0, VK_WHOLE_SIZE);

        VkCommandBuffer commandBuffer{};

        VkCommandBufferAllocateInfo commandBufferAllocatInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        commandBufferAllocatInfo.commandBufferCount = 1;
        commandBufferAllocatInfo.commandPool = m_context->getGraphicsCommandPool();
        commandBufferAllocatInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        if (vkAllocateCommandBuffers(m_device, &commandBufferAllocatInfo, &commandBuffer) != VK_SUCCESS)
        {
            vmaDestroyBuffer(m_allocator, stagingBuf, stagingAlloc);
            return Error{ErrorCode::AllocationFailed, "Failed to allocate texture upload command buffer"};
        }

        VkCommandBufferBeginInfo commandBufferBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

        // Updating: every earlier submission on this queue (frames still sampling the old pixels) finishes first
        VkImageMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        barrier.image = m_image;
        barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, m_levels, 0, 1 };
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.oldLayout = currentLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.srcStageMask = currentLayout == VK_IMAGE_LAYOUT_UNDEFINED
            ? VK_PIPELINE_STAGE_2_NONE
            : VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT;

        VkDependencyInfo dependencyInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

        std::vector<VkBufferImageCopy> regions;
        regions.reserve(image.levels);

        VkDeviceSize bufferOffset = 0;

        uint32_t mipWidth = image.width;
        uint32_t mipHeight = image.height;

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

        if (m_generateMips)
            recordMipChain(commandBuffer);

        // For TRANSFER_DST_OPTIMAL → SHADER_READ_ONLY_OPTIMAL, before any later submission samples it:
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        // With generated mips every level but the last was left as the source of the next blit
        std::array<VkImageMemoryBarrier2, 2> finalBarriers{barrier, barrier};
        uint32_t finalBarrierCount = 1;

        if (m_generateMips && m_levels > 1)
        {
            finalBarriers[0].subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, m_levels - 1, 0, 1};
            finalBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            finalBarriers[0].srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            finalBarriers[1].subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, m_levels - 1, 1, 0, 1};
            finalBarrierCount = 2;
        }

        dependencyInfo.imageMemoryBarrierCount = finalBarrierCount;
        dependencyInfo.pImageMemoryBarriers = finalBarriers.data();

        vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

        vkEndCommandBuffer(commandBuffer);

        // The graphics queue, not the transfer one: the barriers above are recorded into a pool owned by the
        // graphics family, and frames submitted later on the same queue are ordered after them
        return m_context->submitUpload(commandBuffer, stagingBuf, stagingAlloc, imageSize);
    }

    void TextureImpl::recordMipChain(VkCommandBuffer commandBuffer)
    {
        int32_t mipWidth = static_cast<int32_t>(m_width);
        int32_t mipHeight = static_cast<int32_t>(m_height);

        for (uint32_t level = 1; level < m_levels; ++level)
        {
            VkImageMemoryBarrier2 toSource{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
            toSource.image = m_image;
            toSource.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 1, 0, 1};
            toSource.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toSource.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toSource.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            toSource.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            toSource.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT;
            toSource.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            toSource.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
            toSource.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

            VkDependencyInfo dependencyInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
            dependencyInfo.imageMemoryBarrierCount = 1;
            dependencyInfo.pImageMemoryBarriers = &toSource;

            vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

            const int32_t nextWidth = std::max(1, mipWidth / 2);
            const int32_t nextHeight = std::max(1, mipHeight / 2);

            VkImageBlit2 region{VK_STRUCTURE_TYPE_IMAGE_BLIT_2};
            region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, 1};
            region.srcOffsets[1] = {mipWidth, mipHeight, 1};
            region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1};
            region.dstOffsets[1] = {nextWidth, nextHeight, 1};

            VkBlitImageInfo2 blit{VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2};
            blit.srcImage = m_image;
            blit.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            blit.dstImage = m_image;
            blit.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            blit.regionCount = 1;
            blit.pRegions = &region;
            blit.filter = VK_FILTER_LINEAR;

            vkCmdBlitImage2(commandBuffer, &blit);

            mipWidth = nextWidth;
            mipHeight = nextHeight;
        }
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