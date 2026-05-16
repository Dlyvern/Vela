#include "RenderGraphImpl.hpp"

#include "Vela/Core/Context.hpp"

#include "Vela/Graphics/Material.hpp"
#include "Vela/Graphics/Mesh.hpp"

#include "MeshImpl.hpp"
#include "MaterialImpl.hpp"
#include "ContextImpl.hpp"

namespace vela::backend
{
    RenderGraphImpl::RenderGraphImpl(core::Context& context) : m_device(context.impl()->getDevice()),
    m_renderPass(context.impl()->getRenderPass()), m_graphicsQueue(context.impl()->getGraphicsQueue()), m_context(context)
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

        createFramebuffers();
    }

    void RenderGraphImpl::createFramebuffers()
    {
        m_framebuffers.resize(m_context.impl()->getSwapchainImages().size());

        for (size_t i = 0; i < m_context.impl()->getSwapchainImages().size(); ++i)
        {
            VkImageView attachments[] = { m_context.impl()->getSwapchainImageViews()[i] };

            VkFramebufferCreateInfo framebufferCI{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            framebufferCI.renderPass = m_renderPass;
            framebufferCI.attachmentCount = 1;
            framebufferCI.pAttachments = attachments;
            framebufferCI.width = m_context.impl()->getSwapchainExtent().width;
            framebufferCI.height = m_context.impl()->getSwapchainExtent().height;
            framebufferCI.layers = 1;

            if (vkCreateFramebuffer(m_device, &framebufferCI, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create framebuffer");
        }
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

        VkRenderPassBeginInfo renderPassBeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearValue;
        renderPassBeginInfo.framebuffer = m_framebuffers[m_currentImageIndex];
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = m_context.impl()->getSwapchainExtent();
        renderPassBeginInfo.renderPass = m_renderPass;

        vkCmdBeginRenderPass(m_currentCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.width = static_cast<float>(m_context.impl()->getSwapchainExtent().width);
        viewport.height = static_cast<float>(m_context.impl()->getSwapchainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(m_currentCommandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = m_context.impl()->getSwapchainExtent();
        vkCmdSetScissor(m_currentCommandBuffer, 0, 1, &scissor);
    }

    void RenderGraphImpl::endRenderPass()
    {
        if(!m_isFrameValid)
            return;

        vkCmdEndRenderPass(m_currentCommandBuffer);
    }

    void RenderGraphImpl::recreateSwapchainResources()
    {
        vkDeviceWaitIdle(m_device);

        for (auto fb : m_framebuffers) 
            vkDestroyFramebuffer(m_device, fb, nullptr);

        m_framebuffers.clear();

        m_context.impl()->recreateSwapchain();

        createFramebuffers();
    }

    void RenderGraphImpl::draw(const graphics::Mesh& mesh, const graphics::Material& material)
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

        vkCmdDraw(m_currentCommandBuffer, mesh.impl()->getVertexCount(), 1, 0, 0);
    }

    RenderGraphImpl::~RenderGraphImpl()
    {
        vkDeviceWaitIdle(m_device);

        vkDestroySemaphore(m_device, m_imageAvailable, nullptr);
        vkDestroySemaphore(m_device, m_renderFinished, nullptr);
        vkDestroyFence(m_device, m_inFlightFence, nullptr);

        for (auto& frameBuffer : m_framebuffers) 
            vkDestroyFramebuffer(m_device, frameBuffer, nullptr);

    }

} //namespace vela::backend