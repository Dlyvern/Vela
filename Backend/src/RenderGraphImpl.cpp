#include "RenderGraphImpl.hpp"

#include "Vela/Core/Context.hpp"

#include "Vela/Graphics/Material.hpp"
#include "Vela/Graphics/Mesh.hpp"

#include "PresentPass.hpp"
#include "ScenePass.hpp"

#include "MeshImpl.hpp"
#include "MaterialImpl.hpp"
#include "ContextImpl.hpp"

#include <array>
#include <stdexcept>
#include <iostream>
#include <cstring>

struct CameraUBO { vela::math::Mat4 view, projection; };

namespace vela::backend
{
    RenderGraphImpl::RenderGraphImpl(core::Context& context) : m_context(context),
    m_device(context.impl()->getDevice()), m_graphicsQueue(context.impl()->getGraphicsQueue()),
    m_pipelineCache(context.impl()->getDevice())
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

        m_commandBuffers.resize(k_framesInFlight);
        VkCommandBufferAllocateInfo commandBufferAI{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        commandBufferAI.commandPool = context.impl()->getGraphicsCommandPool();
        commandBufferAI.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
        commandBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        if(VkResult result = vkAllocateCommandBuffers(m_device, &commandBufferAI, m_commandBuffers.data()); result != VK_SUCCESS)
            throw std::runtime_error("Failed to allocat command buffers");
        
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
        layouts.fill(context.impl()->getPerViewDescriptorSetLayout());


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

        //TODO for now...
        auto presentPass = std::make_unique<PresentPass>();

        auto scenePass = std::make_unique<ScenePass>();

        addRenderGraphPass("scene", std::move(scenePass));
        addRenderGraphPass("present", std::move(presentPass));

        allocateAllRenderGraphPassOutputs();
    }

    Pass& RenderGraphImpl::addRenderGraphPass(const std::string& name, std::unique_ptr<Pass> pass)
    {
        if (m_renderGraphPassIndices.contains(name))
            throw std::runtime_error("Render graph pass already exists: " + name);

        Pass& added = *pass;

        RenderGraphImpl::RegisteredPass registered;
        registered.declaration = pass->declare();
        registered.formats = formatsOf(registered.declaration);
        registered.pass = std::move(pass);

        m_renderGraphPassIndices[name] = m_renderGraphPasses.size();
        m_renderGraphPasses.push_back(std::move(registered));

        return added;
    }
    
    void RenderGraphImpl::updatePerViewDescriptors(const math::Mat4& view, const math::Mat4& projection)
    {
        CameraUBO mvp{view, projection };
        std::memcpy(m_perViewMapped[m_frameIndex], &mvp, sizeof(CameraUBO));
    }

    void RenderGraphImpl::allocateAllRenderGraphPassOutputs()
    {
        auto& swapchainAttachment = m_attachments["swapchain"] = Attachment{};
        swapchainAttachment.external = true;
        swapchainAttachment.extent = m_context.impl()->getSwapchainExtent();
        swapchainAttachment.format = m_context.impl()->getSwapchainFormat();

        std::unordered_map<std::string, VkImageUsageFlags> usages;

        for(const auto& registered : m_renderGraphPasses)
        {
            for(const auto& colorOutput : registered.declaration.colorOutputs)
                usages[colorOutput.name] |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            if(registered.declaration.depthOutput.has_value())
                usages[registered.declaration.depthOutput->name] |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

            for(const auto& input : registered.declaration.inputs)
                usages[input.name] |= input.usage == InputUsage::TransferSource
                    ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                    : VK_IMAGE_USAGE_SAMPLED_BIT;
        }

        for(const auto& registered : m_renderGraphPasses)
            allocateRenderGraphPassOutputs(registered.declaration, usages);

        for(const auto& attachment : m_attachments)
            m_attachmentLayouts[attachment.first] = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    Attachment RenderGraphImpl::allocateAttachment(const AttachmentOutput& attachmentOutput,
        VkImageUsageFlags usage, bool isDepth)
    {
        const VkExtent2D swapchainExtent = m_context.impl()->getSwapchainExtent();

        Attachment attachment{};
        attachment.extent.width = std::max(1u, static_cast<uint32_t>(swapchainExtent.width * attachmentOutput.scale));
        attachment.extent.height = std::max(1u, static_cast<uint32_t>(swapchainExtent.height * attachmentOutput.scale));
        attachment.format = attachmentOutput.format;

        VkImageCreateInfo imageCI{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.extent = {static_cast<uint32_t>(attachment.extent.width), static_cast<uint32_t>(attachment.extent.height), 1};
        imageCI.mipLevels = 1;
        imageCI.arrayLayers = 1;
        imageCI.format = attachment.format;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL; 
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        imageCI.usage = usage;

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

        VkImageAspectFlags aspectFlags = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        
        imageViewCI.subresourceRange = { aspectFlags, 0, 1, 0, 1 };

        if (VkResult result = vkCreateImageView(m_device, &imageViewCI, nullptr, &attachment.view);
                result != VK_SUCCESS)
            throw std::runtime_error("Failed to create image view");

        return attachment;
    }

    void RenderGraphImpl::allocateRenderGraphPassOutputs(const PassDeclaration& declaration,
        const std::unordered_map<std::string, VkImageUsageFlags>& usages)
    {
        auto allocate = [this, &usages](const AttachmentOutput& output, bool isDepth)
        {
            if(m_attachments.contains(output.name))
                return;

            const auto usageIt = usages.find(output.name);
            const VkImageUsageFlags usage = usageIt != usages.end() ? usageIt->second : 0u;

            m_attachments[output.name] = allocateAttachment(output, usage, isDepth);
        };

        for(const auto& colorOutput : declaration.colorOutputs)
            allocate(colorOutput, false);

        if(declaration.depthOutput.has_value())
            allocate(declaration.depthOutput.value(), true);
    }

    VkExtent2D RenderGraphImpl::passExtent(const PassDeclaration& declaration) const
    {
        const std::string* name = nullptr;

        if(!declaration.colorOutputs.empty())
            name = &declaration.colorOutputs.front().name;
        else if(declaration.depthOutput.has_value())
            name = &declaration.depthOutput->name;

        if(name != nullptr)
            if(const auto it = m_attachments.find(*name); it != m_attachments.end())
                return it->second.extent;

        return m_context.impl()->getSwapchainExtent();
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

    void RenderGraphImpl::resetAllAttachmentLayouts()
    {
        for(auto& [_, layout] : m_attachmentLayouts)
            layout = VK_IMAGE_LAYOUT_UNDEFINED;
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

        Attachment& swapchainAttachment = m_attachments["swapchain"];
        swapchainAttachment.image = m_context.impl()->getSwapchainImages()[m_currentImageIndex];
        swapchainAttachment.view = m_context.impl()->getSwapchainImageViews()[m_currentImageIndex];

        m_currentCommandBuffer = m_commandBuffers[m_frameIndex];

        vkResetCommandBuffer(m_currentCommandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(m_currentCommandBuffer, &beginInfo);

        resetAllAttachmentLayouts();

        //Move swapchain image's layout. For now leave it like this maybe later we can make RGP to render not only in swapchain
        VkImageMemoryBarrier2 toWrite{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        toWrite.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toWrite.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toWrite.image = swapchainAttachment.image;
        toWrite.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        toWrite.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toWrite.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toWrite.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
        toWrite.srcAccessMask = VK_ACCESS_2_NONE;
        toWrite.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
        toWrite.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

        VkDependencyInfo beforeInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        beforeInfo.imageMemoryBarrierCount = 1;
        beforeInfo.pImageMemoryBarriers = &toWrite;

        vkCmdPipelineBarrier2(m_currentCommandBuffer, &beforeInfo);

        m_attachmentLayouts["swapchain"] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }

    void RenderGraphImpl::endFrame()
    {
        if(!m_isFrameValid)
        {
            m_isFrameValid = true;
            return;
        }

        //Move swapchain image's layout
        Attachment& swapchainAttachment = m_attachments["swapchain"];

        VkImageMemoryBarrier2 toPresent{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        toPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        toPresent.image = swapchainAttachment.image;
        toPresent.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toPresent.srcStageMask  = VK_PIPELINE_STAGE_2_BLIT_BIT;
        toPresent.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        toPresent.dstStageMask  = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
        toPresent.dstAccessMask = VK_ACCESS_2_NONE;

        VkDependencyInfo afterInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        afterInfo.imageMemoryBarrierCount = 1;
        afterInfo.pImageMemoryBarriers = &toPresent;

        vkCmdPipelineBarrier2(m_currentCommandBuffer, &afterInfo);

        m_attachmentLayouts["swapchain"] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkSemaphore renderFinished = m_renderFinished[m_currentImageIndex];
        VkFence frameFence = m_inFlightFences[m_frameIndex];

        vkEndCommandBuffer(m_currentCommandBuffer);

        vkResetFences(m_device, 1, &frameFence);

        VkCommandBufferSubmitInfo commandBufferSubmitInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
        commandBufferSubmitInfo.commandBuffer = m_currentCommandBuffer;

        VkSemaphoreSubmitInfo waitSemaphoreSubmitInfo{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        waitSemaphoreSubmitInfo.semaphore = m_imageAvailable[m_frameIndex];
        waitSemaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;

        VkSemaphoreSubmitInfo signalSemaphoreSubmitInfo{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        signalSemaphoreSubmitInfo.semaphore = renderFinished;
        signalSemaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;

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

    void RenderGraphImpl::beginPass(const std::string& renderGraphPassName)
    {
        if(!m_isFrameValid)
            return;

        auto itIndex = m_renderGraphPassIndices.find(renderGraphPassName);

        if(itIndex == m_renderGraphPassIndices.end())
        {
            std::cerr << "Failed to find " << renderGraphPassName << " render graph pass\n";
            return;
        }

        m_currentRenderGraphPass = &m_renderGraphPasses.at(itIndex->second);

        const PassDeclaration& passDeclaration = m_currentRenderGraphPass->declaration;
        const VkExtent2D extent = passExtent(passDeclaration);

        std::vector<VkRenderingAttachmentInfo> colorRenderingAttachmentInfos;
        VkRenderingAttachmentInfo depthRenderingAttachmentInfo{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};

        VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
        renderingInfo.renderArea = {{0, 0}, extent};
        renderingInfo.layerCount = 1;

        std::vector<VkImageMemoryBarrier2> memoryBarriers;

        for(const auto& colorOutput : passDeclaration.colorOutputs)
        {
            const auto outputAttachmentIt = m_attachments.find(colorOutput.name);

            if(outputAttachmentIt == m_attachments.end())
            {
                std::cerr << "Failed to find " << colorOutput.name << " attachment\n";
                continue;
            }

            const Attachment& outputAttachment = outputAttachmentIt->second;

            VkRenderingAttachmentInfo renderingAttachmentInfo{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            renderingAttachmentInfo.imageView = outputAttachment.view;
            renderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            renderingAttachmentInfo.loadOp = colorOutput.load;
            renderingAttachmentInfo.storeOp = colorOutput.store;
            renderingAttachmentInfo.clearValue = colorOutput.clear;

            colorRenderingAttachmentInfos.push_back(renderingAttachmentInfo);

            VkImageMemoryBarrier2 imageBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
            imageBarrier.oldLayout = m_attachmentLayouts[colorOutput.name];
            imageBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            imageBarrier.image = outputAttachment.image;
            imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            imageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
            imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
            imageBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

            m_attachmentLayouts[colorOutput.name] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            memoryBarriers.push_back(imageBarrier);
        }

        if(passDeclaration.depthOutput.has_value())
        {
            const AttachmentOutput& depthOutput = passDeclaration.depthOutput.value();
            const auto outputAttachmentIt = m_attachments.find(depthOutput.name);

            if(outputAttachmentIt == m_attachments.end())
                std::cerr << "Failed to find " << depthOutput.name << " attachment\n";
            else
            {
                const Attachment& outputAttachment = outputAttachmentIt->second;

                depthRenderingAttachmentInfo.imageView = outputAttachment.view;
                depthRenderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                depthRenderingAttachmentInfo.loadOp = depthOutput.load;
                depthRenderingAttachmentInfo.storeOp = depthOutput.store;
                depthRenderingAttachmentInfo.clearValue = depthOutput.clear;

                renderingInfo.pDepthAttachment = &depthRenderingAttachmentInfo;

                VkImageMemoryBarrier2 imageBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
                imageBarrier.oldLayout = m_attachmentLayouts[depthOutput.name];
                imageBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                imageBarrier.image = outputAttachment.image;
                imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageBarrier.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
                imageBarrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
                                    | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                imageBarrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
                                    | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

                m_attachmentLayouts[depthOutput.name] = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

                memoryBarriers.push_back(imageBarrier);
            }
        }

        for(const auto& input : passDeclaration.inputs)
        {
            const auto& inputAttachmentIt = m_attachments.find(input.name);

            if(inputAttachmentIt == m_attachments.end())
            {
                std::cerr << "Failed to find " << input.name << " attachment\n";
                continue;
            }

            const auto& inputAttachment = inputAttachmentIt->second;

            if(input.usage == InputUsage::TransferSource)
            {
                VkImageMemoryBarrier2 toRead{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
                toRead.oldLayout = m_attachmentLayouts[input.name];
                toRead.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                toRead.image = inputAttachment.image;
                toRead.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                toRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                toRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                toRead.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                toRead.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                toRead.dstStageMask  = VK_PIPELINE_STAGE_2_BLIT_BIT;
                toRead.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                memoryBarriers.push_back(toRead);

                m_attachmentLayouts[input.name] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            }
        }

        renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorRenderingAttachmentInfos.size());
        renderingInfo.pColorAttachments = colorRenderingAttachmentInfos.data();

        VkDependencyInfo dependencyInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(memoryBarriers.size());
        dependencyInfo.pImageMemoryBarriers = memoryBarriers.data();

        vkCmdPipelineBarrier2(m_currentCommandBuffer, &dependencyInfo);

        m_currentPassOpenedRendering = renderingInfo.colorAttachmentCount > 0
            || renderingInfo.pDepthAttachment != nullptr;

        if(m_currentPassOpenedRendering)
        {
            VkViewport viewport{};
            viewport.width = static_cast<float>(extent.width);
            viewport.height = static_cast<float>(extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(m_currentCommandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.extent = extent;
            vkCmdSetScissor(m_currentCommandBuffer, 0, 1, &scissor);

            vkCmdBeginRendering(m_currentCommandBuffer, &renderingInfo);
        }

        PassContext passContext(m_attachments);
        passContext.commandBuffer = m_currentCommandBuffer;
        passContext.extent = extent;

        m_currentRenderGraphPass->pass->record(passContext);
    }

    void RenderGraphImpl::endPass()
    {
        if(!m_isFrameValid || m_currentRenderGraphPass == nullptr)
            return;

        if(m_currentPassOpenedRendering)
            vkCmdEndRendering(m_currentCommandBuffer);

        m_currentPassOpenedRendering = false;
        m_currentRenderGraphPass = nullptr;
    }

    void RenderGraphImpl::recreateSwapchainResources()
    {
        vkDeviceWaitIdle(m_device);

        m_context.impl()->recreateSwapchain();

        destroySwapchainSyncObjects();
        createSwapchainSyncObjects();

        //TODO Destroy resources that only depend on swapchain size
        destroyAllAttachments();

        allocateAllRenderGraphPassOutputs();
    }
    
    void RenderGraphImpl::destroyAllAttachments()
    {
        for(auto& [_, attachment] : m_attachments)
        {
            if(attachment.external)
                continue;

            vkDestroyImageView(m_device, attachment.view, nullptr);
            vmaDestroyImage(m_context.impl()->getAllocator(), attachment.image, attachment.allocation);
            attachment.view = VK_NULL_HANDLE;
            attachment.image = VK_NULL_HANDLE;
        }

        m_attachments.clear();
    }

    void RenderGraphImpl::draw(const graphics::Mesh& mesh, const graphics::Material& material, const math::Mat4& model)
    {
        if(!m_isFrameValid || m_currentRenderGraphPass == nullptr)
            return;

        const PipelineDescription& pipelineDescription = material.impl()->getPipelineDescription();

        VkPipeline pipeline = m_pipelineCache.get(pipelineDescription, m_currentRenderGraphPass->formats);

        vkCmdBindPipeline(m_currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        VkPipelineLayout pipelineLayout = pipelineDescription.layout;

        std::array<VkDescriptorSet, 2> descriptorSets
        {
            m_perViewDescriptorSets[m_frameIndex],
            VK_NULL_HANDLE
        };

        uint32_t descriptorSetCount = 1;

        if(VkDescriptorSet materialSet = material.impl()->getDescriptorSet(); materialSet != VK_NULL_HANDLE)
        {
            descriptorSets[1] = materialSet;
            descriptorSetCount = 2;
        }

        vkCmdBindDescriptorSets(m_currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout, 0, descriptorSetCount, descriptorSets.data(), 0, nullptr);

        VkBuffer buffers[] = { mesh.impl()->getBuffer()};
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_currentCommandBuffer, 0, 1, buffers, offsets);

        vkCmdPushConstants(m_currentCommandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(math::Mat4),
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
