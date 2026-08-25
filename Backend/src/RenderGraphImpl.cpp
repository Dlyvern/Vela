#include "RenderGraphImpl.hpp"

#include "Vela/Core/Context.hpp"

#include "Vela/Graphics/Material.hpp"
#include "Vela/Graphics/Mesh.hpp"

#include "MeshImpl.hpp"
#include "MaterialImpl.hpp"
#include "ContextImpl.hpp"

#include <array>
#include <stdexcept>
#include <iostream>
#include <cstring>

struct CameraUBO { glm::mat4 view, projection; };


namespace vela::backend
{
    RenderGraphImpl::RenderGraphImpl(core::Context& context) : m_context(context),
    m_device(context.impl()->getDevice()), m_graphicsQueue(context.impl()->getGraphicsQueue()),
    m_layoutCache(context.impl()->getDevice()), m_pipelineCache(context.impl()->getDevice())
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

        buildRenderGraphPasses();

        m_commandBuffers.resize(k_framesInFlight);
        VkCommandBufferAllocateInfo commandBufferAI{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        commandBufferAI.commandPool = context.impl()->getGraphicsCommandPool();
        commandBufferAI.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
        commandBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        if(VkResult result = vkAllocateCommandBuffers(m_device, &commandBufferAI, m_commandBuffers.data()); result != VK_SUCCESS)
            throw std::runtime_error("Failed to allocat command buffers");
        
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorCount = 1;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        m_perViewDescriptorSetLayout = getLayoutCache().getDescriptorSetLayout({binding});

        std::vector<VkDescriptorPoolSize> poolSizes(1);
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = k_framesInFlight;

        VkDescriptorPoolCreateInfo descriptorPoolCI{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolCI.pPoolSizes = poolSizes.data();
        descriptorPoolCI.maxSets = k_framesInFlight;

        if(VkResult result = vkCreateDescriptorPool(m_device, &descriptorPoolCI, nullptr, &m_descriptorPool); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor pool");

        std::array<VkDescriptorSetLayout, k_framesInFlight> layouts{};
        layouts.fill(m_perViewDescriptorSetLayout);


        VkDescriptorSetAllocateInfo descriptorSetAI{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        descriptorSetAI.descriptorPool = m_descriptorPool;
        descriptorSetAI.descriptorSetCount = static_cast<uint32_t>(m_perViewDescriptorSets.size());
        descriptorSetAI.pSetLayouts = layouts.data();

        if(VkResult result = vkAllocateDescriptorSets(m_device, &descriptorSetAI, m_perViewDescriptorSets.data()); result != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate descriptor sets");

        VkDeviceSize imageSize = sizeof(CameraUBO);

        VkBufferCreateInfo mvpBufferCI{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        mvpBufferCI.size = imageSize;
        mvpBufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        mvpBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo mvpBufferAllocationCI{};
        mvpBufferAllocationCI.usage = VMA_MEMORY_USAGE_AUTO;
        mvpBufferAllocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                            | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo info{};

        std::array<VkDescriptorBufferInfo, k_framesInFlight> bufferInfos{};
        std::array<VkWriteDescriptorSet, k_framesInFlight> writes{};

        for(size_t index = 0; index < writes.size(); ++index)
        {
            if(VkResult result = vmaCreateBuffer(context.impl()->getAllocator(), &mvpBufferCI, &mvpBufferAllocationCI,
            &m_perViewBuffer[index], &m_perViewBufferAllocation[index], &info); result != VK_SUCCESS)
                throw std::runtime_error("Faild to allocate buffer");

            m_perViewMapped[index] = info.pMappedData;

            bufferInfos[index].buffer = m_perViewBuffer[index];
            bufferInfos[index].offset = 0;
            bufferInfos[index].range = sizeof(CameraUBO);

            writes[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[index].dstSet = m_perViewDescriptorSets[index];
            writes[index].dstBinding = 0;
            writes[index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[index].descriptorCount = 1;
            writes[index].pBufferInfo = &bufferInfos[index];
        }

        vkUpdateDescriptorSets(m_device, writes.size(), writes.data(), 0, nullptr);
    }
    
    void RenderGraphImpl::updatePerViewDescriptors(const glm::mat4& view, const glm::mat4& projection)
    {
        CameraUBO mvp{view, projection };
        std::memcpy(m_perViewMapped[m_frameIndex], &mvp, sizeof(CameraUBO));
    }

    void RenderGraphImpl::buildRenderGraphPasses()
    {
        m_presentPass = std::make_unique<PresentPass>(m_context.impl()->getSwapchainFormat());

        allocateRenderGraphPassOutputs(*m_presentPass);
    }

    void RenderGraphImpl::allocateRenderGraphPassOutputs(const Pass& pass)
    {
        const auto& outputs = pass.outputs();

        for(const auto& output : outputs)
        {
            if(auto it = m_attachments.find(output.name); it != m_attachments.end())
            {
                std::cerr << "Attachment with " << output.name << " name already exists! Skiping it\n";
                continue;
            }

            Attachment attachment{};
            attachment.extent.width = m_context.impl()->getSwapchainExtent().width * output.scale;
            attachment.extent.height = m_context.impl()->getSwapchainExtent().height * output.scale;
            attachment.format = output.format;

            VkImageCreateInfo imageCI{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            imageCI.extent = {static_cast<uint32_t>(attachment.extent.width), static_cast<uint32_t>(attachment.extent.height), 1};
            imageCI.mipLevels = 1;
            imageCI.arrayLayers = 1;
            imageCI.format = attachment.format;
            imageCI.tiling = VK_IMAGE_TILING_OPTIMAL; 
            imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCI.usage = output.usage;
            imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo imageAllocCI{};
            imageAllocCI.usage = VMA_MEMORY_USAGE_AUTO;

            if (vmaCreateImage(m_context.impl()->getAllocator(), &imageCI, &imageAllocCI, 
            &attachment.image, &attachment.allocation, nullptr) != VK_SUCCESS)
                throw std::runtime_error("Failed to create depth image");

            VkImageViewCreateInfo imageViewCI{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            imageViewCI.format = attachment.format;
            imageViewCI.image = attachment.image;
            imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCI.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };

            VkImageAspectFlags aspectFlags = m_presentPass->getDepthFormat() ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            
            imageViewCI.subresourceRange = { aspectFlags, 0, 1, 0, 1 };

            if (VkResult result = vkCreateImageView(m_device, &imageViewCI, nullptr, &attachment.view);
                    result != VK_SUCCESS)
                throw std::runtime_error("Failed to create image view");

            m_attachments[output.name] = attachment;
        }
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
        PassContext passContext{.attachments = m_attachments};

        passContext.commandBuffer = m_currentCommandBuffer;
        passContext.colorImage = m_context.impl()->getSwapchainImages()[m_currentImageIndex];
        passContext.colorImageView = m_context.impl()->getSwapchainImageViews().at(m_currentImageIndex);
        passContext.extent = m_context.impl()->getSwapchainExtent();

        return passContext;
    }

    const std::vector<VkFormat>& RenderGraphImpl::getColorFormats() const
    {
        return m_presentPass->getColorFormats();
    }

    LayoutCache& RenderGraphImpl::getLayoutCache()
    {
        return m_layoutCache;
    }

    VkDescriptorSetLayout RenderGraphImpl::getPerViewDescriptorSetLayout() const
    {
        return m_perViewDescriptorSetLayout;
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
        
        //TODO Destroy resources that only depend on swapchain size
        destroyAllAttachments();

        allocateRenderGraphPassOutputs(*m_presentPass);
    }
    
    void RenderGraphImpl::destroyAllAttachments()
    {
        for(auto& [_, attachment] : m_attachments)
        {
            vkDestroyImageView(m_device, attachment.view, nullptr);
            vmaDestroyImage(m_context.impl()->getAllocator(), attachment.image, attachment.allocation);
            attachment.view = VK_NULL_HANDLE;
            attachment.image = VK_NULL_HANDLE;
        }

        m_attachments.clear();
    }

    void RenderGraphImpl::draw(const graphics::Mesh& mesh, const graphics::Material& material, const glm::mat4& model)
    {
        if(!m_isFrameValid)
            return;

        const PipelineDescription& pipelineDescription = material.impl()->getPipelineDescription();

        VkPipeline pipeline = m_pipelineCache.get(pipelineDescription, *m_presentPass);

        vkCmdBindPipeline(m_currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        VkPipelineLayout pipelineLayout = pipelineDescription.layout;

        const std::array<VkDescriptorSet, 2> descriptorSets
        {
            m_perViewDescriptorSets[m_frameIndex],
            material.impl()->getDescriptorSet()
        };

        vkCmdBindDescriptorSets(m_currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

        VkBuffer buffers[] = { mesh.impl()->getBuffer()};
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_currentCommandBuffer, 0, 1, buffers, offsets);

        vkCmdPushConstants(m_currentCommandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
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

        for (size_t index = 0; index < m_perViewBuffer.size(); ++index)
            vmaDestroyBuffer(m_context.impl()->getAllocator(), m_perViewBuffer[index], m_perViewBufferAllocation[index]);

        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

        destroyAllAttachments();
    }

} //namespace vela::backend
