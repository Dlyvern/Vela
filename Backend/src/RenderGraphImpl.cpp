#include "RenderGraphImpl.hpp"

#include "Vela/Core/Context.hpp"

#include "Vela/Graphics/Material.hpp"
#include "Vela/Graphics/Mesh.hpp"

#include "MeshImpl.hpp"
#include "MaterialImpl.hpp"
#include "ContextImpl.hpp"
#include <vulkan/vulkan_core.h>

namespace vela::backend
{
    RenderGraphImpl::RenderGraphImpl(core::Context& context) : m_device(context.impl()->getDevice()), 
    m_graphicsQueue(context.impl()->getGraphicsQueue()), m_context(context)
    {
        VkSemaphoreCreateInfo semaphoreCI{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fenceCI{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if(VkResult result = vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &m_imageAvailable); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create semaphore");

        if(VkResult result = vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &m_renderFinished); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create semaphore");

        if(VkResult result = vkCreateFence(m_device, &fenceCI, nullptr, &m_inFlightFence); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create fence");

        m_commandBuffers.resize(context.impl()->getSwapchainImages().size());
        VkCommandBufferAllocateInfo commandBufferAI{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        commandBufferAI.commandPool = context.impl()->getGraphicsCommandPool();
        commandBufferAI.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
        commandBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        if(VkResult result = vkAllocateCommandBuffers(m_device, &commandBufferAI, m_commandBuffers.data()); result != VK_SUCCESS)
            throw std::runtime_error("Failed to allocat command buffers");
    }

    void RenderGraphImpl::beginFrame()
    {
        vkWaitForFences(m_device, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_device, 1, &m_inFlightFence);

        VkResult acquireResult = vkAcquireNextImageKHR(m_device, m_context.impl()->getSwapchain(), UINT64_MAX, m_imageAvailable, VK_NULL_HANDLE, &m_currentImageIndex);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapchainResources();
            m_isFrameValid = false;

            return;
        }

        m_currentCommandBuffer = m_commandBuffers[m_currentImageIndex];

        vkResetCommandBuffer(m_currentCommandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(m_currentCommandBuffer, &beginInfo);
    }

    void RenderGraphImpl::endFrame()
    {
        if(!m_isFrameValid)
        {
            m_isFrameValid = true;
            return;
        }

        vkEndCommandBuffer(m_currentCommandBuffer);

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &m_imageAvailable;
        submit.pWaitDstStageMask = &waitStage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &m_currentCommandBuffer;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &m_renderFinished;

        vkQueueSubmit(m_graphicsQueue, 1, &submit, m_inFlightFence);

        auto swapchain = m_context.impl()->getSwapchain();

        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &m_renderFinished;
        present.swapchainCount = 1;
        present.pSwapchains = &swapchain;
        present.pImageIndices = &m_currentImageIndex;

        VkResult presentResult = vkQueuePresentKHR(m_graphicsQueue, &present);

        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
            recreateSwapchainResources();
    }

    void RenderGraphImpl::beginPresentPass(float r, float g, float b, float a)
    {
        if(!m_isFrameValid)
            return;

        VkClearValue clearValue{{{r, g, b, a}}};

        VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        colorAttachment.imageView = m_context.impl()->getSwapchainImageViews().at(m_currentImageIndex);
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = clearValue;

        VkRenderingAttachmentInfo depthAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        depthAttachment.imageView = m_context.impl()->getDepthImageView();
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue.depthStencil = {1.0f, 0};

        VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
        renderingInfo.renderArea = {{0,0}, m_context.impl()->getSwapchainExtent()};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment;

        VkViewport viewport{};
        viewport.width = static_cast<float>(m_context.impl()->getSwapchainExtent().width);
        viewport.height = static_cast<float>(m_context.impl()->getSwapchainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(m_currentCommandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = m_context.impl()->getSwapchainExtent();
        vkCmdSetScissor(m_currentCommandBuffer, 0, 1, &scissor);

        VkImageMemoryBarrier toColor{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        toColor.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toColor.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        toColor.srcAccessMask = 0;
        toColor.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        toColor.image = m_context.impl()->getSwapchainImages()[m_currentImageIndex];
        toColor.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        toColor.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toColor.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;


        VkImageMemoryBarrier toDepth{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        toDepth.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toDepth.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        toDepth.srcAccessMask = 0;
        toDepth.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        toDepth.image = m_context.impl()->getDepthImage();
        toDepth.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
        toDepth.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toDepth.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        //This shit allocates every frame, fix it
        std::array<VkImageMemoryBarrier, 2> memoryBarriers{toColor, toDepth};

        vkCmdPipelineBarrier(m_currentCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
             | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        0, 0, nullptr, 0, nullptr, static_cast<uint32_t>(memoryBarriers.size()), memoryBarriers.data());

        vkCmdBeginRendering(m_currentCommandBuffer, &renderingInfo);
    }

    void RenderGraphImpl::endRenderPass()
    {
        if(!m_isFrameValid)
            return;

        vkCmdEndRendering(m_currentCommandBuffer);

        VkImageMemoryBarrier toPresent{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        toPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        toPresent.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        toPresent.dstAccessMask = 0;
        toPresent.image = m_context.impl()->getSwapchainImages()[m_currentImageIndex];
        toPresent.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        vkCmdPipelineBarrier(m_currentCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &toPresent);
    }

    void RenderGraphImpl::recreateSwapchainResources()
    {
        vkDeviceWaitIdle(m_device);
        m_context.impl()->recreateSwapchain();
    }

    void RenderGraphImpl::draw(const graphics::Mesh& mesh, const graphics::Material& material, const glm::mat4& model)
    {
        if(!m_isFrameValid)
            return;

        vkCmdBindPipeline(m_currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material.impl()->getPipeline());

        auto descriptorSet = material.impl()->getDescriptorSet();
        
        vkCmdBindDescriptorSets(m_currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            material.impl()->getPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

        VkBuffer buffers[] = { mesh.impl()->getBuffer()};
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_currentCommandBuffer, 0, 1, buffers, offsets);

        vkCmdPushConstants(m_currentCommandBuffer, material.impl()->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), 
        &model);

        vkCmdDraw(m_currentCommandBuffer, mesh.impl()->getVertexCount(), 1, 0, 0);
    }

    RenderGraphImpl::~RenderGraphImpl()
    {
        vkDeviceWaitIdle(m_device);

        vkDestroySemaphore(m_device, m_imageAvailable, nullptr);
        vkDestroySemaphore(m_device, m_renderFinished, nullptr);
        vkDestroyFence(m_device, m_inFlightFence, nullptr);
    }

} //namespace vela::backend