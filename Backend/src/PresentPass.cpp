#include "PresentPass.hpp"

#include <array>

namespace vela::backend
{
    PresentPass::PresentPass(VkFormat colorFormat, VkFormat depthFormat)
        : m_colorFormats{colorFormat}, m_depthFormat(depthFormat)
    {
    }

    void PresentPass::setClearColor(float r, float g, float b, float a)
    {
        m_clearValue = VkClearValue{{{r, g, b, a}}};
    }

    void PresentPass::setColorFormat(VkFormat colorFormat)
    {
        m_colorFormats[0] = colorFormat;
    }

    const std::vector<VkFormat>& PresentPass::getColorFormats() const
    {
        return m_colorFormats;
    }

    VkFormat PresentPass::getDepthFormat() const
    {
        return m_depthFormat;
    }

    void PresentPass::begin(const PassContext& passContext)
    {
        VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        colorAttachment.imageView = passContext.colorImageView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = m_clearValue;

        VkRenderingAttachmentInfo depthAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        depthAttachment.imageView = passContext.depthImageView;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue.depthStencil = {1.0f, 0};

        VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
        renderingInfo.renderArea = {{0, 0}, passContext.extent};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment;

        VkViewport viewport{};
        viewport.width = static_cast<float>(passContext.extent.width);
        viewport.height = static_cast<float>(passContext.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(passContext.commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = passContext.extent;
        vkCmdSetScissor(passContext.commandBuffer, 0, 1, &scissor);

        VkImageMemoryBarrier2 toColor{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        toColor.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toColor.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        toColor.image = passContext.colorImage;
        toColor.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        toColor.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toColor.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toColor.srcAccessMask = VK_ACCESS_2_NONE;
        toColor.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        toColor.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        toColor.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkImageMemoryBarrier2 toDepth{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        toDepth.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toDepth.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        toDepth.image = passContext.depthImage;
        toDepth.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
        toDepth.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toDepth.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toDepth.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        toDepth.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
                             | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        toDepth.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        toDepth.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
                             | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

        const std::array<VkImageMemoryBarrier2, 2> memoryBarriers{toColor, toDepth};

        VkDependencyInfo dependencyInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(memoryBarriers.size());
        dependencyInfo.pImageMemoryBarriers = memoryBarriers.data();

        vkCmdPipelineBarrier2(passContext.commandBuffer, &dependencyInfo);

        vkCmdBeginRendering(passContext.commandBuffer, &renderingInfo);
    }

    void PresentPass::end(const PassContext& passContext)
    {
        vkCmdEndRendering(passContext.commandBuffer);

        VkImageMemoryBarrier2 toPresent{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        toPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        toPresent.image = passContext.colorImage;
        toPresent.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toPresent.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        toPresent.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        toPresent.dstAccessMask = VK_ACCESS_2_NONE;
        toPresent.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkDependencyInfo dependencyInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &toPresent;

        vkCmdPipelineBarrier2(passContext.commandBuffer, &dependencyInfo);
    }
} //namespace vela::backend
