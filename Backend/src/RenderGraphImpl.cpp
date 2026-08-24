#include "RenderGraphImpl.hpp"

#include "Vela/Core/Context.hpp"

#include "Vela/Graphics/Material.hpp"
#include "Vela/Graphics/Mesh.hpp"

#include "MeshImpl.hpp"
#include "MaterialImpl.hpp"
#include "ContextImpl.hpp"

#include <array>
#include <stdexcept>

namespace vela::backend
{
    RenderGraphImpl::RenderGraphImpl(core::Context& context) : m_context(context),
    m_device(context.impl()->getDevice()), m_graphicsQueue(context.impl()->getGraphicsQueue())
    {
        VkSemaphoreCreateInfo semaphoreCI{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fenceCI{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        m_imageAvailable.resize(k_framesInFlight);
        m_inFlightFences.resize(k_framesInFlight);

        for (uint32_t frame = 0; frame < k_framesInFlight; ++frame)
        {
            if (VkResult result = vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &m_imageAvailable[frame]); result != VK_SUCCESS)
                throw std::runtime_error("Failed to create image-available semaphore");

            if (VkResult result = vkCreateFence(m_device, &fenceCI, nullptr, &m_inFlightFences[frame]); result != VK_SUCCESS)
                throw std::runtime_error("Failed to create fence");
        }

        createSwapchainSyncObjects();

        m_presentPass = std::make_unique<PresentPass>(context.impl()->getSwapchainFormat(),
            context.impl()->getDepthFormat());

        m_commandBuffers.resize(k_framesInFlight);
        VkCommandBufferAllocateInfo commandBufferAI{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        commandBufferAI.commandPool = context.impl()->getGraphicsCommandPool();
        commandBufferAI.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
        commandBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        if(VkResult result = vkAllocateCommandBuffers(m_device, &commandBufferAI, m_commandBuffers.data()); result != VK_SUCCESS)
            throw std::runtime_error("Failed to allocat command buffers");
    }

    void RenderGraphImpl::createSwapchainSyncObjects()
    {
        VkSemaphoreCreateInfo semaphoreCI{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

        m_renderFinished.resize(m_context.impl()->getSwapchainImages().size());

        for (auto& semaphore : m_renderFinished)
            if (VkResult result = vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &semaphore); result != VK_SUCCESS)
                throw std::runtime_error("Failed to create render-finished semaphore");
    }

    void RenderGraphImpl::destroySwapchainSyncObjects()
    {
        for (VkSemaphore semaphore : m_renderFinished)
            vkDestroySemaphore(m_device, semaphore, nullptr);

        m_renderFinished.clear();
    }

    void RenderGraphImpl::beginFrame()
    {
        if (m_context.impl()->isSwapchainStale())
        {
            recreateSwapchainResources();
            m_isFrameValid = false;

            return;
        }

        VkFence frameFence = m_inFlightFences[m_frameIndex];

        vkWaitForFences(m_device, 1, &frameFence, VK_TRUE, UINT64_MAX);

        VkResult acquireResult = vkAcquireNextImageKHR(m_device, m_context.impl()->getSwapchain(), UINT64_MAX,
            m_imageAvailable[m_frameIndex], VK_NULL_HANDLE, &m_currentImageIndex);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapchainResources();
            m_isFrameValid = false;

            return;
        }

        if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("Failed to acquire swapchain image");

        m_currentCommandBuffer = m_commandBuffers[m_frameIndex];

        vkResetCommandBuffer(m_currentCommandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(m_currentCommandBuffer, &beginInfo);
    }

    void RenderGraphImpl::endFrame()
    {
        if(!m_isFrameValid)
        {
            m_isFrameValid = true;
            return;
        }

        VkSemaphore renderFinished = m_renderFinished[m_currentImageIndex];
        VkFence frameFence = m_inFlightFences[m_frameIndex];

        vkEndCommandBuffer(m_currentCommandBuffer);

        vkResetFences(m_device, 1, &frameFence);

        VkCommandBufferSubmitInfo commandBufferSubmitInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
        commandBufferSubmitInfo.commandBuffer = m_currentCommandBuffer;

        VkSemaphoreSubmitInfo waitSemaphoreSubmitInfo{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        waitSemaphoreSubmitInfo.semaphore = m_imageAvailable[m_frameIndex];
        waitSemaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSemaphoreSubmitInfo signalSemaphoreSubmitInfo{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        signalSemaphoreSubmitInfo.semaphore = renderFinished;
        signalSemaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo2 submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
        submitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pWaitSemaphoreInfos = &waitSemaphoreSubmitInfo;
        submitInfo.waitSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = &signalSemaphoreSubmitInfo;
        submitInfo.signalSemaphoreInfoCount = 1;
    
        vkQueueSubmit2(m_graphicsQueue, 1, &submitInfo, frameFence);

        auto swapchain = m_context.impl()->getSwapchain();

        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &renderFinished;
        present.swapchainCount = 1;
        present.pSwapchains = &swapchain;
        present.pImageIndices = &m_currentImageIndex;

        VkResult presentResult = vkQueuePresentKHR(m_graphicsQueue, &present);

        m_frameIndex = (m_frameIndex + 1) % k_framesInFlight;

        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
            recreateSwapchainResources();
    }

    PassContext RenderGraphImpl::makePassContext() const
    {
        PassContext passContext;
        passContext.commandBuffer = m_currentCommandBuffer;
        passContext.colorImage = m_context.impl()->getSwapchainImages()[m_currentImageIndex];
        passContext.colorImageView = m_context.impl()->getSwapchainImageViews().at(m_currentImageIndex);
        passContext.depthImage = m_context.impl()->getDepthImage();
        passContext.depthImageView = m_context.impl()->getDepthImageView();
        passContext.extent = m_context.impl()->getSwapchainExtent();

        return passContext;
    }

    const std::vector<VkFormat>& RenderGraphImpl::getColorFormats() const
    {
        return m_presentPass->getColorFormats();
    }

    VkFormat RenderGraphImpl::getDepthFormat() const
    {
        return m_presentPass->getDepthFormat();
    }

    void RenderGraphImpl::beginPresentPass(float r, float g, float b, float a)
    {
        if(!m_isFrameValid)
            return;

        m_presentPass->setClearColor(r, g, b, a);
        m_presentPass->begin(makePassContext());
    }

    void RenderGraphImpl::endRenderPass()
    {
        if(!m_isFrameValid)
            return;

        m_presentPass->end(makePassContext());
    }

    void RenderGraphImpl::recreateSwapchainResources()
    {
        vkDeviceWaitIdle(m_device);

        m_context.impl()->recreateSwapchain();

        destroySwapchainSyncObjects();
        createSwapchainSyncObjects();

        m_presentPass->setColorFormat(m_context.impl()->getSwapchainFormat());
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

        destroySwapchainSyncObjects();

        for (VkSemaphore semaphore : m_imageAvailable)
            vkDestroySemaphore(m_device, semaphore, nullptr);

        for (VkFence fence : m_inFlightFences)
            vkDestroyFence(m_device, fence, nullptr);
    }

} //namespace vela::backend
