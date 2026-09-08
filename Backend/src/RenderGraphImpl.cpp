#include "RenderGraphImpl.hpp"

#include "Vela/Core/Context.hpp"

#include "Vela/Graphics/Material.hpp"
#include "Vela/Graphics/Mesh.hpp"

#include "PassAdapter.hpp"

#include "MeshImpl.hpp"
#include "MaterialImpl.hpp"
#include "ContextImpl.hpp"

#include <array>
#include <chrono>
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

        VkQueryPoolCreateInfo queryPoolCI{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        queryPoolCI.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolCI.queryCount = k_framesInFlight * 2;

        if(VkResult result = vkCreateQueryPool(m_device, &queryPoolCI, nullptr, &m_queryPool); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create query pool");

        m_timestampPeriod = context.impl()->getPhysicalDeviceProperties().limits.timestampPeriod;
        m_gpuTimingSupported = context.impl()->getGraphicsTimestampValidBits() > 0;

        if(!m_gpuTimingSupported)
            std::cerr << "Graphics queue does not support timestamps, GPU timing unavailable\n";
    }

    Status RenderGraphImpl::addPass(const std::string& name, std::unique_ptr<graphics::Pass> pass)
    {
        if (pass == nullptr)
            return Error{ErrorCode::InvalidArgument, "Render graph pass must not be null"};

        if (m_renderGraphPassIndices.contains(name))
            return Error{ErrorCode::InvalidArgument, "Render graph pass already exists: " + name};

        addRenderGraphPass(name, std::make_unique<PassAdapter>(std::move(pass), *this));

        return {};
    }

    void RenderGraphImpl::buildRenderGraphOrder()
    {
        const size_t passCount = m_renderGraphPasses.size();

        m_attachmentProducers.clear();
        m_executionOrder.clear();
        m_executionOrder.reserve(passCount);

        for (size_t index = 0; index < passCount; ++index)
        {
            const PassDeclaration& declaration = m_renderGraphPasses[index].declaration;

            for (const AttachmentOutput& colorOutput : declaration.colorOutputs)
                m_attachmentProducers[colorOutput.name].push_back(index);

            if (declaration.depthOutput.has_value())
                m_attachmentProducers[declaration.depthOutput->name].push_back(index);
        }

        std::vector<std::vector<size_t>> successors(passCount);
        std::vector<size_t> inDegree(passCount, 0);

        for (size_t consumer = 0; consumer < passCount; ++consumer)
        {
            for (const PassInput& input : m_renderGraphPasses[consumer].declaration.inputs)
            {
                const auto producersIt = m_attachmentProducers.find(input.name);

                if (producersIt == m_attachmentProducers.end())
                    continue;

                for (size_t producer : producersIt->second)
                {
                    if (producer == consumer)
                        continue;

                    successors[producer].push_back(consumer);
                    ++inDegree[consumer];
                }
            }
        }

        std::vector<bool> emitted(passCount, false);

        while (m_executionOrder.size() < passCount)
        {
            size_t next = passCount;

            for (size_t index = 0; index < passCount; ++index)
            {
                if (!emitted[index] && inDegree[index] == 0)
                {
                    next = index;
                    break;
                }
            }

            if (next == passCount)
                break;

            emitted[next] = true;
            m_executionOrder.push_back(next);

            for (size_t successor : successors[next])
                --inDegree[successor];
        }

        if (m_executionOrder.size() != passCount)
        {
            auto nameOf = [this](size_t index)
            {
                for (const auto& [name, passIndex] : m_renderGraphPassIndices)
                    if (passIndex == index)
                        return name;

                return std::string("<unnamed>");
            };

            std::string message = "Render graph contains a cycle involving:";

            for (size_t index = 0; index < passCount; ++index)
                if (!emitted[index])
                    message += " " + nameOf(index);

            throw std::runtime_error(message);
        }

        cullRenderGraphOrder(successors);
    }

    void RenderGraphImpl::cullRenderGraphOrder(const std::vector<std::vector<size_t>>& successors)
    {
        if (m_presentAttachmentName.empty())
            return;

        const auto rootsIt = m_attachmentProducers.find(m_presentAttachmentName);

        if (rootsIt == m_attachmentProducers.end())
            return;

        const size_t passCount = m_renderGraphPasses.size();

        std::vector<std::vector<size_t>> predecessors(passCount);

        for (size_t producer = 0; producer < passCount; ++producer)
            for (size_t consumer : successors[producer])
                predecessors[consumer].push_back(producer);

        std::vector<bool> live(passCount, false);
        std::vector<size_t> pending = rootsIt->second;

        for (size_t root : pending)
            live[root] = true;

        while (!pending.empty())
        {
            const size_t current = pending.back();
            pending.pop_back();

            for (size_t predecessor : predecessors[current])
            {
                if (live[predecessor])
                    continue;

                live[predecessor] = true;
                pending.push_back(predecessor);
            }
        }

        std::vector<size_t> culled;
        culled.reserve(m_executionOrder.size());

        for (size_t index : m_executionOrder)
            if (live[index])
                culled.push_back(index);

        m_executionOrder = std::move(culled);
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

        m_isRenderGraphDirty = true;

        return added;
    }
    
    void RenderGraphImpl::setView(const math::Mat4& view, const math::Mat4& projection)
    {
        m_view = view;
        m_projection = projection;
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

        if(!m_presentAttachmentName.empty())
            usages[m_presentAttachmentName] |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        for(const auto& registered : m_renderGraphPasses)
            allocateRenderGraphPassOutputs(registered.declaration, usages);

        for(const auto& attachment : m_attachments)
            m_attachmentLayouts[attachment.first] = VK_IMAGE_LAYOUT_UNDEFINED;

        buildRenderGraphOrder();
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

    Status RenderGraphImpl::beginFrame()
    {
        if (m_context.impl()->isSwapchainStale())
        {
            recreateSwapchainResources();
            m_isFrameValid = false;

            return {};
        }

        VkFence frameFence = m_inFlightFences[m_frameIndex];

        vkWaitForFences(m_device, 1, &frameFence, VK_TRUE, UINT64_MAX);

        if (m_gpuTimingSupported && m_timestampsWritten[m_frameIndex])
        {
            uint64_t results[4]{};

            vkGetQueryPoolResults(m_device, m_queryPool, m_frameIndex * 2, 2,
                sizeof(results), results, sizeof(uint64_t) * 2,
                VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

            if (results[1] != 0 && results[3] != 0)
            {
                const uint64_t delta = results[2] - results[0];
                m_frameStats.gpuFrameMs = static_cast<float>(delta) * m_timestampPeriod / 1'000'000.0f;
                m_frameStats.gpuTimingValid = true;
            }
        }


        VkResult acquireResult = vkAcquireNextImageKHR(m_device, m_context.impl()->getSwapchain(), UINT64_MAX,
            m_imageAvailable[m_frameIndex], VK_NULL_HANDLE, &m_currentImageIndex);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapchainResources();
            m_isFrameValid = false;

            return {};
        }

        if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
        {
            m_isFrameValid = false;

            return Error{ErrorCode::SwapchainCreationFailed, "Failed to acquire swapchain image"};
        }

        Attachment& swapchainAttachment = m_attachments["swapchain"];
        swapchainAttachment.image = m_context.impl()->getSwapchainImages()[m_currentImageIndex];
        swapchainAttachment.view = m_context.impl()->getSwapchainImageViews()[m_currentImageIndex];

        m_currentCommandBuffer = m_commandBuffers[m_frameIndex];

        vkResetCommandBuffer(m_currentCommandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(m_currentCommandBuffer, &beginInfo);

        if (m_gpuTimingSupported)
        {
            vkCmdResetQueryPool(m_currentCommandBuffer, m_queryPool, m_frameIndex * 2, 2);
            vkCmdWriteTimestamp2(m_currentCommandBuffer, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, m_queryPool, m_frameIndex * 2);
            m_timestampsWritten[m_frameIndex] = true;
        }

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

        return {};
    }

    graphics::FrameStats RenderGraphImpl::getFrameStats() const
    {
        return m_frameStats;
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

        if (m_gpuTimingSupported)
            vkCmdWriteTimestamp2(m_currentCommandBuffer, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, m_queryPool, m_frameIndex * 2 + 1);

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

    Status RenderGraphImpl::execute()
    {
        const auto frameStart = std::chrono::steady_clock::now();

        if(m_presentAttachmentName.empty())
            return Error{ErrorCode::InvalidArgument, "No present source set on the render graph"};

        if(m_isRenderGraphDirty)
        {
            destroyAllAttachments();
            allocateAllRenderGraphPassOutputs();
            buildRenderGraphOrder();
            m_isRenderGraphDirty = false;
        }

        const auto presentIt = m_attachments.find(m_presentAttachmentName);

        if(presentIt == m_attachments.end())
            return Error{ErrorCode::InvalidArgument,
                "Present source attachment not found: " + m_presentAttachmentName};

        if (auto acquired = beginFrame(); !acquired)
            return acquired.error();

        if (m_isFrameValid)
        {
            CameraUBO mvp{m_view, m_projection};

            std::memcpy(m_perViewMapped[m_frameIndex], &mvp, sizeof(CameraUBO));
            vmaFlushAllocation(m_context.impl()->getAllocator(),
                m_perViewBufferAllocation[m_frameIndex], 0, VK_WHOLE_SIZE);

            for (size_t index : m_executionOrder)
                runPass(m_renderGraphPasses[index]);

            const Attachment& presentAttachment = presentIt->second;

            VkImageMemoryBarrier2 toRead{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
            toRead.oldLayout = m_attachmentLayouts[m_presentAttachmentName];
            toRead.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            toRead.image = presentAttachment.image;
            toRead.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            toRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toRead.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            toRead.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            toRead.dstStageMask  = VK_PIPELINE_STAGE_2_BLIT_BIT;
            toRead.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

            m_attachmentLayouts[m_presentAttachmentName] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

            VkDependencyInfo dependencyInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
            dependencyInfo.imageMemoryBarrierCount = 1;
            dependencyInfo.pImageMemoryBarriers = &toRead;

            vkCmdPipelineBarrier2(m_currentCommandBuffer, &dependencyInfo);

            const Attachment& swapchain = m_attachments["swapchain"];

            VkImageBlit2 region{VK_STRUCTURE_TYPE_IMAGE_BLIT_2};
            region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            region.srcOffsets[0] = {0, 0, 0};
            region.srcOffsets[1] = {static_cast<int32_t>(presentAttachment.extent.width),
                                    static_cast<int32_t>(presentAttachment.extent.height), 1};
            region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            region.dstOffsets[0] = {0, 0, 0};
            region.dstOffsets[1] = {static_cast<int32_t>(m_context.impl()->getSwapchainExtent().width),
                                    static_cast<int32_t>(m_context.impl()->getSwapchainExtent().height), 1};

            VkBlitImageInfo2 blit{VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2};
            blit.srcImage = presentAttachment.image;
            blit.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            blit.dstImage = swapchain.image;
            blit.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            blit.regionCount = 1;
            blit.pRegions = &region;
            blit.filter = VK_FILTER_LINEAR;

            vkCmdBlitImage2(m_currentCommandBuffer, &blit);
        }

        endFrame();

        m_frameStats.cpuFrameMs = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - frameStart).count();

        return {};
    }

    void RenderGraphImpl::setPresentSource(std::string attachmentName)
    {
        m_presentAttachmentName = std::move(attachmentName);
        m_isRenderGraphDirty = true;
    }

    void RenderGraphImpl::runPass(RegisteredPass& registered)
    {
        if(!m_isFrameValid)
            return;

        m_currentRenderGraphPass = &registered;

        m_boundMaterial = nullptr;
        m_boundPipeline = VK_NULL_HANDLE;
        m_boundPipelineLayout = VK_NULL_HANDLE;
        m_boundVertexBuffer = VK_NULL_HANDLE;
        m_boundIndexBuffer = VK_NULL_HANDLE;

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

        const MaterialImpl* materialImpl = material.impl();

        if(materialImpl != m_boundMaterial)
        {
            const PipelineDescription& pipelineDescription = materialImpl->getPipelineDescription();
            VkPipeline pipeline = m_pipelineCache.get(pipelineDescription, m_currentRenderGraphPass->formats);

            if(pipeline != m_boundPipeline)
            {
                vkCmdBindPipeline(m_currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                m_boundPipeline = pipeline;
            }

            m_boundPipelineLayout = pipelineDescription.layout;

            std::array<VkDescriptorSet, 2> descriptorSets
            {
                m_perViewDescriptorSets[m_frameIndex],
                VK_NULL_HANDLE
            };

            uint32_t descriptorSetCount = 1;

            if(VkDescriptorSet materialSet = materialImpl->getDescriptorSet(); materialSet != VK_NULL_HANDLE)
            {
                descriptorSets[1] = materialSet;
                descriptorSetCount = 2;
            }

            vkCmdBindDescriptorSets(m_currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_boundPipelineLayout, 0, descriptorSetCount, descriptorSets.data(), 0, nullptr);

            m_boundMaterial = materialImpl;
        }

        if(VkBuffer vertexBuffer = mesh.impl()->getVertexBuffer(); vertexBuffer != m_boundVertexBuffer)
        {
            VkBuffer buffers[] = { vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(m_currentCommandBuffer, 0, 1, buffers, offsets);

            m_boundVertexBuffer = vertexBuffer;
        }

        vkCmdPushConstants(m_currentCommandBuffer, m_boundPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(math::Mat4),
        &model);

        if (mesh.impl()->isIndexed())
        {
            if (VkBuffer indexBuffer = mesh.impl()->getIndexBuffer(); indexBuffer != m_boundIndexBuffer)
            {
                vkCmdBindIndexBuffer(m_currentCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                m_boundIndexBuffer = indexBuffer;
            }

            vkCmdDrawIndexed(m_currentCommandBuffer, mesh.impl()->getIndexCount(), 1, 0, 0, 0);
        }
        else
        {
            vkCmdDraw(m_currentCommandBuffer, mesh.impl()->getVertexCount(), 1, 0, 0);
        }
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

        vkDestroyQueryPool(m_device, m_queryPool, nullptr);

        destroyAllAttachments();
    }

} //namespace vela::backend
