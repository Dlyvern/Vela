#include "RenderGraphImpl.hpp"
#include "Vela/Log.hpp"

#include "Vela/Core/Context.hpp"

#include "Vela/Graphics/Material.hpp"
#include "Vela/Graphics/Mesh.hpp"
#include "Vela/Graphics/DynamicBuffer.hpp"

#include "Vela/Graphics/ComputeProgram.hpp"
#include "Vela/Graphics/Texture.hpp"

#include "PassAdapter.hpp"
#include "ComputePassAdapter.hpp"
#include "ComputeProgramImpl.hpp"
#include "DynamicBufferImpl.hpp"

#include "MeshImpl.hpp"
#include "MaterialImpl.hpp"
#include "ContextImpl.hpp"
#include "TextureImpl.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <optional>

namespace
{
    // std140: three mat4 followed by a vec4, no padding needed
    struct CameraUBO
    {
        vela::math::Mat4 view;
        vela::math::Mat4 projection;
        vela::math::Mat4 viewProjection;
        float cameraPosition[4];
    };

    constexpr uint32_t k_transientSetsPerPool{256};

    constexpr VkAccessFlags2 k_writeAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
        | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
        | VK_ACCESS_2_TRANSFER_WRITE_BIT
        | VK_ACCESS_2_SHADER_WRITE_BIT
        | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT
        | VK_ACCESS_2_MEMORY_WRITE_BIT;
} //namespace

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
        queryPoolCI.queryCount = k_framesInFlight * k_queriesPerFrame;

        if(VkResult result = vkCreateQueryPool(m_device, &queryPoolCI, nullptr, &m_queryPool); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create query pool");

        m_timestampPeriod = context.impl()->getPhysicalDeviceProperties().limits.timestampPeriod;
        m_gpuTimingSupported = context.impl()->getGraphicsTimestampValidBits() > 0;

        if(!m_gpuTimingSupported)
            detail::LogLine(LogLevel::Warning) << "Graphics queue does not support timestamps, GPU timing unavailable\n";
    }

    Status RenderGraphImpl::addPass(const std::string& name, std::unique_ptr<graphics::Pass> pass)
    {
        if (pass == nullptr)
            return Error{ErrorCode::InvalidArgument, "Render graph pass must not be null"};

        if (m_renderGraphPassIndices.contains(name))
            return Error{ErrorCode::InvalidArgument, "Render graph pass already exists: " + name};

        return addRenderGraphPass(name, std::make_unique<PassAdapter>(std::move(pass), *this));
    }

    Status RenderGraphImpl::addComputePass(const std::string& name, std::unique_ptr<graphics::ComputePass> pass)
    {
        if (pass == nullptr)
            return Error{ErrorCode::InvalidArgument, "Compute pass must not be null"};

        if (m_renderGraphPassIndices.contains(name))
            return Error{ErrorCode::InvalidArgument, "Render graph pass already exists: " + name};

        return addRenderGraphPass(name, std::make_unique<ComputePassAdapter>(std::move(pass), *this));
    }

    Status RenderGraphImpl::removePass(const std::string& name)
    {
        if (m_currentRenderGraphPass != nullptr)
            return Error{ErrorCode::InvalidArgument, "Render graph passes can't be removed while the graph records"};

        const auto indexIt = m_renderGraphPassIndices.find(name);

        if (indexIt == m_renderGraphPassIndices.end())
            return Error{ErrorCode::InvalidArgument, "Render graph pass does not exist: " + name};

        const size_t removed = indexIt->second;

        m_renderGraphPasses.erase(m_renderGraphPasses.begin() + static_cast<std::ptrdiff_t>(removed));
        m_renderGraphPassIndices.erase(indexIt);

        for (auto& [passName, index] : m_renderGraphPassIndices)
            if (index > removed)
                --index;

        m_isRenderGraphDirty = true;

        return {};
    }

    Status RenderGraphImpl::redescribePass(const std::string& name)
    {
        if (m_currentRenderGraphPass != nullptr)
            return Error{ErrorCode::InvalidArgument, "Render graph passes can't be redescribed while the graph records"};

        const auto indexIt = m_renderGraphPassIndices.find(name);

        if (indexIt == m_renderGraphPassIndices.end())
            return Error{ErrorCode::InvalidArgument, "Render graph pass does not exist: " + name};

        RegisteredPass& registered = m_renderGraphPasses[indexIt->second];

        PassDeclaration declaration = registered.pass->declare();

        if (auto valid = validateDeclaration(name, declaration); !valid)
            return valid;

        registered.declaration = std::move(declaration);
        registered.formats = formatsOf(registered.declaration);

        m_isRenderGraphDirty = true;

        return {};
    }

    void RenderGraphImpl::bindComputeProgram(const graphics::ComputeProgram& program)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass)
            return;

        const ComputeProgramImpl* programImpl = program.impl();

        vkCmdBindPipeline(m_currentCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, programImpl->getPipeline());

        m_boundComputeProgram = &program;
        m_boundPipelineLayout = programImpl->getPipelineLayout();
        m_boundPushConstantSize = programImpl->getPushConstantSize();
        m_boundPushConstantStages = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    void RenderGraphImpl::bindStorageBuffer(const graphics::ComputeProgram& program, uint32_t slot,
        const std::string& bufferName)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass)
            return;

        const auto bufferIt = m_buffers.find(bufferName);

        if (bufferIt == m_buffers.end())
        {
            detail::LogLine(LogLevel::Error) << "Compute pass referenced unknown buffer " << bufferName << '\n';
            return;
        }

        if (auto bound = program.impl()->setStorageBuffer(slot, bufferIt->second.buffer, bufferIt->second.size); !bound)
        {
            detail::LogLine(LogLevel::Error) << bound.error().message << '\n';
            return;
        }

        VkDescriptorSet descriptorSet = program.impl()->getDescriptorSet();

        if (descriptorSet == VK_NULL_HANDLE)
            return;

        vkCmdBindDescriptorSets(m_currentCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            program.impl()->getPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
    }

    void RenderGraphImpl::drawInstanced(uint32_t vertexCount, uint32_t instanceCount,
        uint32_t firstVertex, uint32_t firstInstance)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass)
            return;

        flushDescriptors();

        vkCmdDraw(m_currentCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void RenderGraphImpl::bindAttachment(uint32_t slot, const std::string& attachmentName,
        const graphics::SamplerDescription* sampler)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass)
            return;

        if (slot >= k_passInputSlots)
        {
            detail::LogLine(LogLevel::Error) << "Pass input slot " << slot << " is out of range\n";
            return;
        }

        const auto attachmentIt = m_attachments.find(attachmentName);

        if (attachmentIt == m_attachments.end())
        {
            detail::LogLine(LogLevel::Error) << "Pass referenced unknown attachment " << attachmentName << '\n';
            return;
        }

        if (attachmentIt->second.samples != VK_SAMPLE_COUNT_1_BIT)
        {
            detail::LogLine(LogLevel::Error) << "Attachment " << attachmentName << " is multisampled; bind its resolve target instead\n";
            return;
        }

        graphics::SamplerDescription samplerDescription{};
        samplerDescription.addressU = graphics::AddressMode::ClampToEdge;
        samplerDescription.addressV = graphics::AddressMode::ClampToEdge;
        samplerDescription.addressW = graphics::AddressMode::ClampToEdge;

        // Linear filtering of depth formats is optional in Vulkan
        if (attachmentIt->second.format == VK_FORMAT_D32_SFLOAT)
        {
            samplerDescription.minFilter = graphics::Filter::Nearest;
            samplerDescription.magFilter = graphics::Filter::Nearest;
            samplerDescription.mipMode = graphics::MipMode::Nearest;
        }

        if (sampler != nullptr)
            samplerDescription = *sampler;

        InputBinding binding{};
        binding.view = attachmentIt->second.view;
        binding.sampler = m_context.impl()->getSamplerCache().getSampler(samplerDescription);

        if (m_inputBindings[slot].view == binding.view && m_inputBindings[slot].sampler == binding.sampler)
            return;

        m_inputBindings[slot] = binding;
        m_inputBindingsDirty = true;
    }

    void RenderGraphImpl::bindTexture(uint32_t slot, const graphics::Texture& texture)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass)
            return;

        if (slot >= k_passInputSlots)
        {
            detail::LogLine(LogLevel::Error) << "Pass input slot " << slot << " is out of range\n";
            return;
        }

        InputBinding binding{};
        binding.view = texture.impl()->getImageView();
        binding.sampler = texture.impl()->getSampler();

        if (m_inputBindings[slot].view == binding.view && m_inputBindings[slot].sampler == binding.sampler)
            return;

        m_inputBindings[slot] = binding;
        m_inputBindingsDirty = true;
    }

    VkDescriptorPool RenderGraphImpl::createTransientPool()
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = k_transientSetsPerPool * k_passInputSlots;

        VkDescriptorPoolCreateInfo poolCI{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        poolCI.poolSizeCount = 1;
        poolCI.pPoolSizes = &poolSize;
        poolCI.maxSets = k_transientSetsPerPool;

        VkDescriptorPool pool{VK_NULL_HANDLE};

        if (vkCreateDescriptorPool(m_device, &poolCI, nullptr, &pool) != VK_SUCCESS)
            return VK_NULL_HANDLE;

        return pool;
    }

    VkDescriptorSet RenderGraphImpl::allocateTransientSet(VkDescriptorSetLayout layout)
    {
        std::vector<VkDescriptorPool>& pools = m_transientPools[m_frameIndex];
        size_t& cursor = m_transientPoolCursor[m_frameIndex];

        while (true)
        {
            if (cursor >= pools.size())
            {
                VkDescriptorPool pool = createTransientPool();

                if (pool == VK_NULL_HANDLE)
                {
                    detail::LogLine(LogLevel::Error) << "Failed to create transient descriptor pool\n";
                    return VK_NULL_HANDLE;
                }

                pools.push_back(pool);
            }

            VkDescriptorSetAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            allocateInfo.descriptorPool = pools[cursor];
            allocateInfo.descriptorSetCount = 1;
            allocateInfo.pSetLayouts = &layout;

            VkDescriptorSet set{VK_NULL_HANDLE};
            const VkResult result = vkAllocateDescriptorSets(m_device, &allocateInfo, &set);

            if (result == VK_SUCCESS)
                return set;

            if (result != VK_ERROR_OUT_OF_POOL_MEMORY && result != VK_ERROR_FRAGMENTED_POOL)
            {
                detail::LogLine(LogLevel::Error) << "Failed to allocate transient descriptor set\n";
                return VK_NULL_HANDLE;
            }

            ++cursor;
        }
    }

    void RenderGraphImpl::flushDescriptors()
    {
        if (m_boundPipelineLayout == VK_NULL_HANDLE || m_boundMaterial == nullptr)
            return;

        bool rebindInputs = false;

        if (m_materialSetsDirty)
        {
            std::array<VkDescriptorSet, 2> descriptorSets
            {
                m_perViewDescriptorSets[m_frameIndex],
                VK_NULL_HANDLE
            };

            uint32_t descriptorSetCount = 1;

            if (VkDescriptorSet materialSet = m_boundMaterial->getDescriptorSet(m_frameIndex); materialSet != VK_NULL_HANDLE)
            {
                descriptorSets[1] = materialSet;
                descriptorSetCount = 2;
            }

            vkCmdBindDescriptorSets(m_currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_boundPipelineLayout, 0, descriptorSetCount, descriptorSets.data(), 0, nullptr);

            m_materialSetsDirty = false;

            // A different pipeline layout disturbs everything after set 1
            rebindInputs = true;
        }

        if (m_inputBindingsDirty)
        {
            m_inputBindingsDirty = false;
            m_currentInputSet = allocateTransientSet(m_context.impl()->getPassInputDescriptorSetLayout());

            if (m_currentInputSet != VK_NULL_HANDLE)
            {
                std::array<VkDescriptorImageInfo, k_passInputSlots> imageInfos{};
                std::array<VkWriteDescriptorSet, k_passInputSlots> writes{};
                uint32_t writeCount = 0;

                for (uint32_t slot = 0; slot < k_passInputSlots; ++slot)
                {
                    if (m_inputBindings[slot].view == VK_NULL_HANDLE)
                        continue;

                    imageInfos[writeCount].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfos[writeCount].imageView = m_inputBindings[slot].view;
                    imageInfos[writeCount].sampler = m_inputBindings[slot].sampler;

                    VkWriteDescriptorSet& write = writes[writeCount];
                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write.dstSet = m_currentInputSet;
                    write.dstBinding = slot;
                    write.descriptorCount = 1;
                    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    write.pImageInfo = &imageInfos[writeCount];

                    ++writeCount;
                }

                vkUpdateDescriptorSets(m_device, writeCount, writes.data(), 0, nullptr);
            }

            rebindInputs = true;
        }

        if (rebindInputs && m_currentInputSet != VK_NULL_HANDLE)
            vkCmdBindDescriptorSets(m_currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_boundPipelineLayout, k_passInputSet, 1, &m_currentInputSet, 0, nullptr);
    }

    void RenderGraphImpl::bindMaterialStorageBuffer(uint32_t slot, const std::string& bufferName)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass || m_boundMaterial == nullptr)
            return;

        const auto bufferIt = m_buffers.find(bufferName);

        if (bufferIt == m_buffers.end())
        {
            detail::LogLine(LogLevel::Error) << "Pass referenced unknown buffer " << bufferName << '\n';
            return;
        }

        if (auto bound = m_boundMaterial->setStorageBuffer(slot, bufferIt->second.buffer, bufferIt->second.size); !bound)
            detail::LogLine(LogLevel::Error) << bound.error().message << '\n';

        m_materialSetsDirty = true;
    }

    void RenderGraphImpl::setComputeConstants(const math::Mat4& value)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass)
            return;

        pushConstants(&value, sizeof(math::Mat4));
    }

    void RenderGraphImpl::dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass || m_boundComputeProgram == nullptr)
            return;

        vkCmdDispatch(m_currentCommandBuffer, groupsX, groupsY, groupsZ);
    }

    void RenderGraphImpl::allocateGraphBuffers()
    {
        std::unordered_map<std::string, VkDeviceSize> sizes;

        for (const auto& registered : m_renderGraphPasses)
            for (const BufferOutput& output : registered.declaration.bufferOutputs)
                sizes[output.name] = std::max(sizes[output.name], output.size);

        for (const auto& [name, size] : sizes)
        {
            if (m_buffers.contains(name) || size == 0)
                continue;

            VkBufferCreateInfo bufferCI{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            bufferCI.size = size;
            bufferCI.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocationCI{};
            allocationCI.usage = VMA_MEMORY_USAGE_AUTO;

            GraphBuffer graphBuffer{};
            graphBuffer.size = size;

            if (vmaCreateBuffer(m_context.impl()->getAllocator(), &bufferCI, &allocationCI,
                    &graphBuffer.buffer, &graphBuffer.allocation, nullptr) != VK_SUCCESS)
            {
                detail::LogLine(LogLevel::Error) << "Failed to allocate graph buffer " << name << '\n';
                continue;
            }

            m_buffers[name] = graphBuffer;
        }
    }

    void RenderGraphImpl::transitionGraphBuffers(const PassDeclaration& declaration, PassKind kind)
    {
        std::vector<VkBufferMemoryBarrier2> bufferBarriers;

        auto transition = [&](const std::string& name, VkPipelineStageFlags2 stage, VkAccessFlags2 access)
        {
            const auto bufferIt = m_buffers.find(name);

            if (bufferIt == m_buffers.end())
                return;

            GraphBuffer& graphBuffer = bufferIt->second;

            if (graphBuffer.lastStage == stage && graphBuffer.lastAccess == access)
                return;

            if (graphBuffer.lastStage != VK_PIPELINE_STAGE_2_NONE)
            {
                VkBufferMemoryBarrier2 bufferBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
                bufferBarrier.srcStageMask = graphBuffer.lastStage;
                bufferBarrier.srcAccessMask = graphBuffer.lastAccess;
                bufferBarrier.dstStageMask = stage;
                bufferBarrier.dstAccessMask = access;
                bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                bufferBarrier.buffer = graphBuffer.buffer;
                bufferBarrier.offset = 0;
                bufferBarrier.size = VK_WHOLE_SIZE;

                bufferBarriers.push_back(bufferBarrier);
            }

            graphBuffer.lastStage = stage;
            graphBuffer.lastAccess = access;
        };

        const VkPipelineStageFlags2 stage = kind == PassKind::Compute
            ? VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
            : VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;

        for (const BufferOutput& bufferOutput : declaration.bufferOutputs)
            transition(bufferOutput.name, stage, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT);

        for (const BufferInput& bufferInput : declaration.bufferInputs)
            transition(bufferInput.name, stage, bufferInput.access == BufferAccess::ReadWrite
                ? VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT
                : VK_ACCESS_2_SHADER_STORAGE_READ_BIT);

        if (bufferBarriers.empty())
            return;

        VkDependencyInfo dependencyInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependencyInfo.bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers.size());
        dependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();

        vkCmdPipelineBarrier2(m_currentCommandBuffer, &dependencyInfo);
    }

    void RenderGraphImpl::destroyAllGraphBuffers()
    {
        DeletionQueue& deletionQueue = m_context.impl()->getDeletionQueue();

        for (auto& [name, graphBuffer] : m_buffers)
            if (graphBuffer.buffer != VK_NULL_HANDLE)
                deletionQueue.push({.buffer = graphBuffer.buffer, .allocation = graphBuffer.allocation});

        m_buffers.clear();
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
            {
                m_attachmentProducers[colorOutput.name].push_back(index);

                if (!colorOutput.resolveTarget.empty())
                    m_attachmentProducers[colorOutput.resolveTarget].push_back(index);
            }

            if (declaration.depthOutput.has_value())
            {
                m_attachmentProducers[declaration.depthOutput->name].push_back(index);

                if (!declaration.depthOutput->resolveTarget.empty())
                    m_attachmentProducers[declaration.depthOutput->resolveTarget].push_back(index);
            }

            for (const BufferOutput& bufferOutput : declaration.bufferOutputs)
                m_attachmentProducers[bufferOutput.name].push_back(index);
        }

        std::vector<std::vector<size_t>> successors(passCount);
        std::vector<size_t> inDegree(passCount, 0);

        auto addEdges = [&](size_t consumer, const std::string& resourceName)
        {
            const auto producersIt = m_attachmentProducers.find(resourceName);

            if (producersIt == m_attachmentProducers.end())
                return;

            for (size_t producer : producersIt->second)
            {
                if (producer == consumer)
                    continue;

                successors[producer].push_back(consumer);
                ++inDegree[consumer];
            }
        };

        for (size_t consumer = 0; consumer < passCount; ++consumer)
        {
            const PassDeclaration& declaration = m_renderGraphPasses[consumer].declaration;

            for (const PassInput& input : declaration.inputs)
                addEdges(consumer, input.name);

            for (const BufferInput& bufferInput : declaration.bufferInputs)
                addEdges(consumer, bufferInput.name);

            // Loading an attachment means drawing on top of what the passes added before this one wrote into it
            auto addLoadEdges = [&](const AttachmentOutput& output)
            {
                if (output.load != VK_ATTACHMENT_LOAD_OP_LOAD)
                    return;

                for (size_t producer : m_attachmentProducers[output.name])
                {
                    if (producer >= consumer)
                        continue;

                    successors[producer].push_back(consumer);
                    ++inDegree[consumer];
                }
            };

            for (const AttachmentOutput& colorOutput : declaration.colorOutputs)
                addLoadEdges(colorOutput);

            if (declaration.depthOutput.has_value())
                addLoadEdges(declaration.depthOutput.value());
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

    Status RenderGraphImpl::validateDeclaration(const std::string& passName, const PassDeclaration& declaration) const
    {
        const uint32_t maxSamples = m_context.impl()->getDeviceInfo().maxMsaaSamples;

        std::vector<const AttachmentOutput*> outputs;

        for (const AttachmentOutput& colorOutput : declaration.colorOutputs)
            outputs.push_back(&colorOutput);

        if (declaration.depthOutput.has_value())
            outputs.push_back(&declaration.depthOutput.value());

        for (const AttachmentOutput* output : outputs)
        {
            const uint32_t samples = static_cast<uint32_t>(output->samples);

            if (samples == 0 || (samples & (samples - 1)) != 0 || samples > maxSamples)
                return Error{ErrorCode::InvalidArgument, "Pass " + passName + ": attachment " + output->name + " asks for "
                    + std::to_string(samples) + " samples; the device supports powers of two up to " + std::to_string(maxSamples)};

            if (output->samples != outputs.front()->samples)
                return Error{ErrorCode::InvalidArgument, "Pass " + passName
                    + ": every color and depth output of a pass must use the same sample count"};

            if (!output->resolveTarget.empty() && output->samples == VK_SAMPLE_COUNT_1_BIT)
                return Error{ErrorCode::InvalidArgument, "Pass " + passName + ": attachment " + output->name
                    + " has a resolve target but is not multisampled"};

            if (output->resolveTarget == output->name)
                return Error{ErrorCode::InvalidArgument, "Pass " + passName + ": attachment " + output->name
                    + " can't resolve into itself"};
        }

        return {};
    }

    Status RenderGraphImpl::addRenderGraphPass(const std::string& name, std::unique_ptr<Pass> pass)
    {
        RenderGraphImpl::RegisteredPass registered;
        registered.name = name;
        registered.declaration = pass->declare();

        if (auto valid = validateDeclaration(name, registered.declaration); !valid)
            return valid;

        registered.formats = formatsOf(registered.declaration);
        registered.pass = std::move(pass);

        m_renderGraphPassIndices[name] = m_renderGraphPasses.size();
        m_renderGraphPasses.push_back(std::move(registered));

        m_isRenderGraphDirty = true;

        return {};
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
            {
                usages[colorOutput.name] |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

                if(!colorOutput.resolveTarget.empty())
                    usages[colorOutput.resolveTarget] |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }

            if(registered.declaration.depthOutput.has_value())
            {
                usages[registered.declaration.depthOutput->name] |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

                if(!registered.declaration.depthOutput->resolveTarget.empty())
                    usages[registered.declaration.depthOutput->resolveTarget] |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }

            for(const auto& input : registered.declaration.inputs)
                usages[input.name] |= input.usage == InputUsage::TransferSource
                    ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                    : VK_IMAGE_USAGE_SAMPLED_BIT;
        }

        if(!m_presentAttachmentName.empty())
            usages[m_presentAttachmentName] |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        for(const auto& registered : m_renderGraphPasses)
            allocateRenderGraphPassOutputs(registered.declaration, usages);

        constexpr VkImageUsageFlags k_readUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        for(const auto& [name, attachment] : m_attachments)
            if(attachment.samples != VK_SAMPLE_COUNT_1_BIT && (usages[name] & k_readUsage) != 0)
                throw std::runtime_error("Attachment " + name + " is multisampled and can't be a pass input or the "
                    "present source; read its resolve target instead");

        m_attachmentStates.clear();

        for(const auto& attachment : m_attachments)
            m_attachmentStates[attachment.first] = AttachmentState{};

        buildRenderGraphOrder();
    }

    Attachment RenderGraphImpl::allocateAttachment(const AttachmentOutput& attachmentOutput,
        VkImageUsageFlags usage, bool isDepth)
    {
        const VkExtent2D swapchainExtent = m_context.impl()->getSwapchainExtent();

        Attachment attachment{};

        if(attachmentOutput.size.width <= 0 && attachmentOutput.size.height <= 0)
        {
            attachment.extent.width = std::max(1u, static_cast<uint32_t>(swapchainExtent.width * attachmentOutput.scale));
            attachment.extent.height = std::max(1u, static_cast<uint32_t>(swapchainExtent.height * attachmentOutput.scale));
        }
        else
        {
            attachment.extent = attachmentOutput.size;
            attachment.isFixedSize = true;
        }

        attachment.format = attachmentOutput.format;
        attachment.samples = attachmentOutput.samples;

        VkImageCreateInfo imageCI{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.extent = {static_cast<uint32_t>(attachment.extent.width), static_cast<uint32_t>(attachment.extent.height), 1};
        imageCI.mipLevels = 1;
        imageCI.arrayLayers = 1;
        imageCI.format = attachment.format;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL; 
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        imageCI.usage = usage;

        imageCI.samples = attachment.samples;
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

        auto allocateResolveTarget = [&allocate](const AttachmentOutput& output, bool isDepth)
        {
            if(output.resolveTarget.empty())
                return;

            AttachmentOutput resolveTarget = output;
            resolveTarget.name = output.resolveTarget;
            resolveTarget.resolveTarget.clear();
            resolveTarget.samples = VK_SAMPLE_COUNT_1_BIT;

            allocate(resolveTarget, isDepth);
        };

        for(const auto& colorOutput : declaration.colorOutputs)
        {
            allocate(colorOutput, false);
            allocateResolveTarget(colorOutput, false);
        }

        if(declaration.depthOutput.has_value())
        {
            allocate(declaration.depthOutput.value(), true);
            allocateResolveTarget(declaration.depthOutput.value(), true);
        }
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
        for(auto& [_, state] : m_attachmentStates)
            state = AttachmentState{};
    }

    void RenderGraphImpl::transitionAttachment(const std::string& name, VkImageLayout newLayout,
        VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess, std::vector<VkImageMemoryBarrier2>& barriers)
    {
        const auto attachmentIt = m_attachments.find(name);

        if (attachmentIt == m_attachments.end())
            return;

        AttachmentState& state = m_attachmentStates[name];

        // Read after read in the same layout: nothing to wait for
        if (state.layout == newLayout && (state.access & k_writeAccess) == 0 && (dstAccess & k_writeAccess) == 0)
        {
            state.stage |= dstStage;
            state.access |= dstAccess;
            return;
        }

        const Attachment& attachment = attachmentIt->second;
        const bool isDepth = attachment.format == VK_FORMAT_D32_SFLOAT;

        VkImageMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        barrier.oldLayout = state.layout;
        barrier.newLayout = newLayout;
        barrier.image = attachment.image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange = {isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        barrier.dstStageMask = dstStage;
        barrier.dstAccessMask = dstAccess;

        if (state.layout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            /*
                First use this frame. Attachments are shared by every frame in flight, so the previous
                frame may still be reading or writing this image on the GPU
            */
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        }
        else
        {
            barrier.srcStageMask = state.stage;
            barrier.srcAccessMask = state.access & k_writeAccess;
        }

        barriers.push_back(barrier);

        state.layout = newLayout;
        state.stage = dstStage;
        state.access = dstAccess;
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

        const auto waitStart = std::chrono::steady_clock::now();

        vkWaitForFences(m_device, 1, &frameFence, VK_TRUE, UINT64_MAX);

        m_frameWaitMs = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - waitStart).count();

        m_context.impl()->getDeletionQueue().collect();
        m_context.impl()->reclaimUploads(false);

        for (VkDescriptorPool pool : m_transientPools[m_frameIndex])
            vkResetDescriptorPool(m_device, pool, 0);

        m_transientPoolCursor[m_frameIndex] = 0;

        if (m_gpuTimingSupported && m_timestampsWritten[m_frameIndex])
            readTimestamps();

        deliverReadback(m_frameIndex, true);

        const auto acquireStart = std::chrono::steady_clock::now();

        VkResult acquireResult = vkAcquireNextImageKHR(m_device, m_context.impl()->getSwapchain(), UINT64_MAX,
            m_imageAvailable[m_frameIndex], VK_NULL_HANDLE, &m_currentImageIndex);

        m_frameWaitMs += std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - acquireStart).count();

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
            vkCmdResetQueryPool(m_currentCommandBuffer, m_queryPool, m_frameIndex * k_queriesPerFrame, k_queriesPerFrame);
            vkCmdWriteTimestamp2(m_currentCommandBuffer, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, m_queryPool,
                m_frameIndex * k_queriesPerFrame);
            m_timestampsWritten[m_frameIndex] = true;
            m_timedPassNames[m_frameIndex].clear();
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

        m_attachmentStates["swapchain"] = {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT};

        return {};
    }

    graphics::FrameStats RenderGraphImpl::getFrameStats() const
    {
        return m_frameStats;
    }

    const std::vector<graphics::PassTiming>& RenderGraphImpl::getPassTimings() const
    {
        return m_passTimings;
    }

    void RenderGraphImpl::readTimestamps()
    {
        const std::vector<std::string>& passNames = m_timedPassNames[m_frameIndex];
        const uint32_t queryCount = 2 + 2 * static_cast<uint32_t>(passNames.size());

        // Each query is a value followed by its availability
        std::array<uint64_t, k_queriesPerFrame * 2> results{};

        vkGetQueryPoolResults(m_device, m_queryPool, m_frameIndex * k_queriesPerFrame, queryCount,
            queryCount * sizeof(uint64_t) * 2, results.data(), sizeof(uint64_t) * 2,
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

        auto elapsedMs = [this, &results](uint32_t beginQuery, uint32_t endQuery) -> std::optional<float>
        {
            if (results[beginQuery * 2 + 1] == 0 || results[endQuery * 2 + 1] == 0)
                return std::nullopt;

            const uint64_t delta = results[endQuery * 2] - results[beginQuery * 2];

            return static_cast<float>(delta) * m_timestampPeriod / 1'000'000.0f;
        };

        if (const std::optional<float> frameMs = elapsedMs(0, 1))
        {
            m_frameStats.gpuFrameMs = *frameMs;
            m_frameStats.gpuTimingValid = true;
        }

        m_passTimings.clear();

        for (uint32_t pass = 0; pass < passNames.size(); ++pass)
            if (const std::optional<float> passMs = elapsedMs(2 + pass * 2, 3 + pass * 2))
                m_passTimings.push_back({passNames[pass], *passMs});
    }

    void RenderGraphImpl::writePassTimestamp(bool begin)
    {
        if (!m_gpuTimingSupported || m_currentRenderGraphPass == nullptr)
            return;

        std::vector<std::string>& passNames = m_timedPassNames[m_frameIndex];

        if (begin)
        {
            if (passNames.size() >= k_maxTimedPasses)
                return;

            passNames.push_back(m_currentRenderGraphPass->name);
        }
        else if (passNames.empty() || passNames.back() != m_currentRenderGraphPass->name)
            return;

        const uint32_t query = m_frameIndex * k_queriesPerFrame + 2 + 2 * static_cast<uint32_t>(passNames.size() - 1)
            + (begin ? 0 : 1);

        vkCmdWriteTimestamp2(m_currentCommandBuffer, begin ? VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT
            : VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, m_queryPool, query);
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

        m_attachmentStates["swapchain"] = {VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_ACCESS_2_NONE};

        VkSemaphore renderFinished = m_renderFinished[m_currentImageIndex];
        VkFence frameFence = m_inFlightFences[m_frameIndex];

        if (m_gpuTimingSupported)
            vkCmdWriteTimestamp2(m_currentCommandBuffer, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, m_queryPool,
                m_frameIndex * k_queriesPerFrame + 1);

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
        m_context.impl()->getDeletionQueue().advanceFrame();

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
            allocateGraphBuffers();
            buildRenderGraphOrder();
            m_isRenderGraphDirty = false;
        }

        if(!m_attachments.contains(m_presentAttachmentName))
            return Error{ErrorCode::InvalidArgument,
                "Present source attachment not found: " + m_presentAttachmentName};

        if (auto acquired = beginFrame(); !acquired)
            return acquired.error();

        // beginFrame may have recreated every attachment after a resize
        const auto presentIt = m_attachments.find(m_presentAttachmentName);

        if (m_isFrameValid && presentIt != m_attachments.end())
        {
            CameraUBO mvp{};
            mvp.view = m_view;
            mvp.projection = m_projection;
            mvp.viewProjection = m_projection * m_view;

            // Eye position is -R^T * t of the view matrix
            for (size_t axis = 0; axis < 3; ++axis)
                mvp.cameraPosition[axis] = -(m_view[axis][0] * m_view[3][0]
                    + m_view[axis][1] * m_view[3][1]
                    + m_view[axis][2] * m_view[3][2]);

            mvp.cameraPosition[3] = 1.0f;

            std::memcpy(m_perViewMapped[m_frameIndex], &mvp, sizeof(CameraUBO));
            vmaFlushAllocation(m_context.impl()->getAllocator(),
                m_perViewBufferAllocation[m_frameIndex], 0, VK_WHOLE_SIZE);

            for (size_t index : m_executionOrder)
                runPass(m_renderGraphPasses[index]);

            const Attachment& presentAttachment = presentIt->second;

            std::vector<VkImageMemoryBarrier2> presentBarriers;
            transitionAttachment(m_presentAttachmentName, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, presentBarriers);

            if (!presentBarriers.empty())
            {
                VkDependencyInfo dependencyInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
                dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(presentBarriers.size());
                dependencyInfo.pImageMemoryBarriers = presentBarriers.data();

                vkCmdPipelineBarrier2(m_currentCommandBuffer, &dependencyInfo);
            }

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

            if (m_pendingReadback)
                recordReadback(presentAttachment);
        }

        endFrame();

        m_frameStats.cpuFrameMs = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - frameStart).count();
        m_frameStats.cpuWaitMs = std::min(m_frameWaitMs, m_frameStats.cpuFrameMs);
        m_frameStats.cpuWorkMs = m_frameStats.cpuFrameMs - m_frameStats.cpuWaitMs;
        m_frameWaitMs = 0.0f;

        return {};
    }

    Status RenderGraphImpl::readbackFrame(std::function<void(const graphics::ImageData&)> onReady)
    {
        if (!onReady)
            return Error{ErrorCode::InvalidArgument, "readbackFrame needs a callback"};

        if (m_pendingReadback)
            return Error{ErrorCode::InvalidArgument, "A frame readback is already pending"};

        for (const Readback& readback : m_readbacks)
            if (readback.onReady)
                return Error{ErrorCode::InvalidArgument, "A frame readback is already pending"};

        m_pendingReadback = std::move(onReady);

        return {};
    }

    void RenderGraphImpl::recordReadback(const Attachment& presentAttachment)
    {
        Readback& readback = m_readbacks[m_frameIndex];
        readback.extent = m_context.impl()->getSwapchainExtent();

        const VmaAllocator allocator = m_context.impl()->getAllocator();

        // Blitting into an 8-bit sRGB image converts the linear present source exactly as presenting does.
        // A UNORM swapchain means the present source already holds sRGB-encoded values: copy those unchanged
        const bool srgbSwapchain = m_context.impl()->getSwapchainInfo().srgb;

        VkImageCreateInfo imageCI{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.extent = {readback.extent.width, readback.extent.height, 1};
        imageCI.mipLevels = 1;
        imageCI.arrayLayers = 1;
        imageCI.format = srgbSwapchain ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo imageAllocationCI{};
        imageAllocationCI.usage = VMA_MEMORY_USAGE_AUTO;

        VkBufferCreateInfo bufferCI{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferCI.size = static_cast<VkDeviceSize>(readback.extent.width) * readback.extent.height * 4;
        bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo bufferAllocationCI{};
        bufferAllocationCI.usage = VMA_MEMORY_USAGE_AUTO;
        bufferAllocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo bufferInfo{};

        if (vmaCreateImage(allocator, &imageCI, &imageAllocationCI, &readback.image, &readback.imageAllocation, nullptr) != VK_SUCCESS
            || vmaCreateBuffer(allocator, &bufferCI, &bufferAllocationCI, &readback.buffer, &readback.bufferAllocation,
                &bufferInfo) != VK_SUCCESS)
        {
            detail::LogLine(LogLevel::Error) << "Failed to allocate frame readback; the capture is skipped\n";
            deliverReadback(m_frameIndex, false);
            m_pendingReadback = nullptr;
            return;
        }

        readback.mapped = bufferInfo.pMappedData;
        readback.onReady = std::move(m_pendingReadback);
        m_pendingReadback = nullptr;

        VkImageMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = readback.image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

        VkDependencyInfo dependencyInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(m_currentCommandBuffer, &dependencyInfo);

        // Same blit as the one to the swapchain, so the capture matches what the window shows
        VkImageBlit2 region{VK_STRUCTURE_TYPE_IMAGE_BLIT_2};
        region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.srcOffsets[1] = {static_cast<int32_t>(presentAttachment.extent.width),
                                static_cast<int32_t>(presentAttachment.extent.height), 1};
        region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.dstOffsets[1] = {static_cast<int32_t>(readback.extent.width), static_cast<int32_t>(readback.extent.height), 1};

        VkBlitImageInfo2 blit{VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2};
        blit.srcImage = presentAttachment.image;
        blit.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blit.dstImage = readback.image;
        blit.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blit.regionCount = 1;
        blit.pRegions = &region;
        blit.filter = VK_FILTER_LINEAR;

        vkCmdBlitImage2(m_currentCommandBuffer, &blit);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier2(m_currentCommandBuffer, &dependencyInfo);

        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {readback.extent.width, readback.extent.height, 1};

        vkCmdCopyImageToBuffer(m_currentCommandBuffer, readback.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            readback.buffer, 1, &copy);

        VkBufferMemoryBarrier2 toHost{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
        toHost.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        toHost.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        toHost.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
        toHost.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
        toHost.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toHost.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toHost.buffer = readback.buffer;
        toHost.size = VK_WHOLE_SIZE;

        VkDependencyInfo hostDependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        hostDependency.bufferMemoryBarrierCount = 1;
        hostDependency.pBufferMemoryBarriers = &toHost;

        vkCmdPipelineBarrier2(m_currentCommandBuffer, &hostDependency);
    }

    void RenderGraphImpl::deliverReadback(uint32_t frameIndex, bool invoke)
    {
        Readback& readback = m_readbacks[frameIndex];
        const VmaAllocator allocator = m_context.impl()->getAllocator();

        if (invoke && readback.onReady && readback.mapped != nullptr)
        {
            vmaInvalidateAllocation(allocator, readback.bufferAllocation, 0, VK_WHOLE_SIZE);

            graphics::ImageData image{};
            image.width = readback.extent.width;
            image.height = readback.extent.height;
            image.format = graphics::TextureFormat::RGBA8Srgb;
            image.pixels = std::span<const std::byte>(static_cast<const std::byte*>(readback.mapped),
                static_cast<size_t>(readback.extent.width) * readback.extent.height * 4);

            readback.onReady(image);
        }

        if (readback.image != VK_NULL_HANDLE)
            vmaDestroyImage(allocator, readback.image, readback.imageAllocation);

        if (readback.buffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(allocator, readback.buffer, readback.bufferAllocation);

        readback = Readback{};
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

        writePassTimestamp(true);

        m_boundMaterial = nullptr;
        m_boundPipeline = VK_NULL_HANDLE;
        m_boundPipelineLayout = VK_NULL_HANDLE;
        m_boundVertexBuffer = VK_NULL_HANDLE;
        m_boundIndexBuffer = VK_NULL_HANDLE;
        m_boundComputeProgram = nullptr;
        m_boundPushConstantSize = 0;
        m_inputBindings = {};
        m_inputBindingsDirty = false;
        m_materialSetsDirty = false;
        m_currentInputSet = VK_NULL_HANDLE;

        if (registered.pass->kind() == PassKind::Compute)
        {
            m_currentPassOpenedRendering = false;

            transitionGraphBuffers(registered.declaration, PassKind::Compute);

            PassContext computeContext(m_attachments);
            computeContext.commandBuffer = m_currentCommandBuffer;

            registered.pass->record(computeContext);

            writePassTimestamp(false);
            m_currentRenderGraphPass = nullptr;

            return;
        }

        const PassDeclaration& passDeclaration = m_currentRenderGraphPass->declaration;

        transitionGraphBuffers(passDeclaration, PassKind::Graphics);

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
                detail::LogLine(LogLevel::Error) << "Failed to find " << colorOutput.name << " attachment\n";
                continue;
            }

            VkRenderingAttachmentInfo renderingAttachmentInfo{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            renderingAttachmentInfo.imageView = outputAttachmentIt->second.view;
            renderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            renderingAttachmentInfo.loadOp = colorOutput.load;
            renderingAttachmentInfo.storeOp = colorOutput.store;
            renderingAttachmentInfo.clearValue = colorOutput.clear;

            if(const auto resolveIt = m_attachments.find(colorOutput.resolveTarget);
                !colorOutput.resolveTarget.empty() && resolveIt != m_attachments.end())
            {
                renderingAttachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
                renderingAttachmentInfo.resolveImageView = resolveIt->second.view;
                renderingAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                transitionAttachment(colorOutput.resolveTarget, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, memoryBarriers);
            }

            colorRenderingAttachmentInfos.push_back(renderingAttachmentInfo);

            transitionAttachment(colorOutput.name, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, memoryBarriers);
        }

        if(passDeclaration.depthOutput.has_value())
        {
            const AttachmentOutput& depthOutput = passDeclaration.depthOutput.value();
            const auto outputAttachmentIt = m_attachments.find(depthOutput.name);

            if(outputAttachmentIt == m_attachments.end())
                detail::LogLine(LogLevel::Error) << "Failed to find " << depthOutput.name << " attachment\n";
            else
            {
                depthRenderingAttachmentInfo.imageView = outputAttachmentIt->second.view;
                depthRenderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                depthRenderingAttachmentInfo.loadOp = depthOutput.load;
                depthRenderingAttachmentInfo.storeOp = depthOutput.store;
                depthRenderingAttachmentInfo.clearValue = depthOutput.clear;

                if(const auto resolveIt = m_attachments.find(depthOutput.resolveTarget);
                    !depthOutput.resolveTarget.empty() && resolveIt != m_attachments.end())
                {
                    // Sample zero is the one depth resolve mode every Vulkan 1.2+ device supports
                    depthRenderingAttachmentInfo.resolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
                    depthRenderingAttachmentInfo.resolveImageView = resolveIt->second.view;
                    depthRenderingAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

                    // Depth resolves run in the color attachment output stage
                    transitionAttachment(depthOutput.resolveTarget, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                        memoryBarriers);
                }

                renderingInfo.pDepthAttachment = &depthRenderingAttachmentInfo;

                transitionAttachment(depthOutput.name, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                    memoryBarriers);
            }
        }

        for(const auto& input : passDeclaration.inputs)
        {
            if(!m_attachments.contains(input.name))
            {
                detail::LogLine(LogLevel::Error) << "Failed to find " << input.name << " attachment\n";
                continue;
            }

            if(input.usage == InputUsage::TransferSource)
                transitionAttachment(input.name, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, memoryBarriers);
            else
                transitionAttachment(input.name, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                    VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, memoryBarriers);
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

        writePassTimestamp(false);

        m_currentPassOpenedRendering = false;
        m_currentRenderGraphPass = nullptr;
    }

    void RenderGraphImpl::recreateSwapchainResources()
    {
        // Minimised: keep the old swapchain and skip frames until the window has a size again
        if (!m_context.impl()->isSurfaceDrawable())
            return;

        vkDeviceWaitIdle(m_device);

        const VkExtent2D previousExtent = m_context.impl()->getSwapchainExtent();

        m_context.impl()->recreateSwapchain();

        /*
            vkDeviceWaitIdle does not cover the presentation engine, which may still be waiting on the
            render-finished semaphores of the old swapchain. Retire them a few frames later instead
        */
        DeletionQueue& deletionQueue = m_context.impl()->getDeletionQueue();

        for (VkSemaphore semaphore : m_renderFinished)
            deletionQueue.push({.semaphore = semaphore});

        m_renderFinished.clear();

        createSwapchainSyncObjects();

        // Only the present mode changed: every attachment still has the right size
        const VkExtent2D extent = m_context.impl()->getSwapchainExtent();

        if (extent.width == previousExtent.width && extent.height == previousExtent.height)
            return;

        destroyAllAttachments(true);

        allocateAllRenderGraphPassOutputs();
    }
    
    void RenderGraphImpl::destroyAllAttachments(bool keepFixedSize)
    {
        DeletionQueue& deletionQueue = m_context.impl()->getDeletionQueue();

        std::erase_if(m_attachments, [&](const auto& entry)
        {
            const Attachment& attachment = entry.second;

            if(attachment.isFixedSize && keepFixedSize)
                return false;

            if(!attachment.external)
                deletionQueue.push({.image = attachment.image,
                                    .imageView = attachment.view,
                                    .allocation = attachment.allocation});

            return true;
        });
    }

    void RenderGraphImpl::bindMaterial(const graphics::Material& material)
    {
        MaterialImpl* materialImpl = material.impl();

        if(materialImpl == m_boundMaterial)
            return;

        const PipelineDescription& pipelineDescription = materialImpl->getPipelineDescription();

        VkPipeline pipeline = m_pipelineCache.get(pipelineDescription, m_currentRenderGraphPass->formats);

        if(pipeline != m_boundPipeline)
        {
            vkCmdBindPipeline(m_currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            m_boundPipeline = pipeline;
        }

        m_boundPipelineLayout = pipelineDescription.layout;
        m_boundPushConstantSize = materialImpl->getPushConstantSize();
        m_boundPushConstantStages = materialImpl->getPushConstantStages();

        m_boundMaterial = materialImpl;
        m_materialSetsDirty = true;
    }

    void RenderGraphImpl::bind(const graphics::Material& material)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass)
            return;

        bindMaterial(material);
    }

    void RenderGraphImpl::bindVertexBuffer(const graphics::DynamicBuffer& buffer)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass)
            return;

        VkBuffer vertexBuffer = buffer.impl()->getBuffer();

        if(!vertexBuffer || vertexBuffer == m_boundVertexBuffer)
            return;

        VkBuffer buffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_currentCommandBuffer, 0, 1, buffers, offsets);
        m_boundVertexBuffer = vertexBuffer;
    }

    void RenderGraphImpl::bindIndexBuffer(const graphics::DynamicBuffer& buffer)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass)
            return;

        VkBuffer indexBuffer = buffer.impl()->getBuffer();

        if(!indexBuffer || indexBuffer == m_boundIndexBuffer)
            return;

        vkCmdBindIndexBuffer(m_currentCommandBuffer, indexBuffer, 0, buffer.impl()->getIndexType());
        m_boundIndexBuffer = indexBuffer;
    }

    void RenderGraphImpl::pushConstants(const void* data, uint32_t size)
    {
        if(m_boundPipelineLayout == VK_NULL_HANDLE || m_boundPushConstantSize == 0)
            return;

        vkCmdPushConstants(m_currentCommandBuffer, m_boundPipelineLayout, m_boundPushConstantStages, 0,
            std::min(size, m_boundPushConstantSize), data);
    }

    void RenderGraphImpl::setConstants(const math::Mat4& value)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass)
            return;

        pushConstants(&value, sizeof(math::Mat4));
    }

    void RenderGraphImpl::setConstantBytes(const void* data, uint32_t size)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass || data == nullptr)
            return;

        pushConstants(data, size);
    }

    void RenderGraphImpl::setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass)
            return;

        VkRect2D scissor{};

        scissor.offset.x = x;
        scissor.offset.y = y;

        scissor.extent.width = width;
        scissor.extent.height = height;

        vkCmdSetScissor(m_currentCommandBuffer, 0, 1, &scissor);
    }

    void RenderGraphImpl::drawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset,
                 uint32_t instanceCount, uint32_t firstInstance)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass)
            return;

        flushDescriptors();

        vkCmdDrawIndexed(m_currentCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void RenderGraphImpl::draw(const graphics::Mesh& mesh, const graphics::Material& material,
        const void* constants, uint32_t size)
    {
        if(!m_isFrameValid || !m_currentRenderGraphPass)
            return;

        bindMaterial(material);

        if(VkBuffer vertexBuffer = mesh.impl()->getVertexBuffer(); vertexBuffer != m_boundVertexBuffer)
        {
            VkBuffer buffers[] = { vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(m_currentCommandBuffer, 0, 1, buffers, offsets);

            m_boundVertexBuffer = vertexBuffer;
        }

        pushConstants(constants, size);

        flushDescriptors();

        if (mesh.impl()->isIndexed())
        {
            if (VkBuffer indexBuffer = mesh.impl()->getIndexBuffer(); indexBuffer != m_boundIndexBuffer)
            {
                vkCmdBindIndexBuffer(m_currentCommandBuffer, indexBuffer, 0, mesh.impl()->getIndexType());
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

        for (const auto& pools : m_transientPools)
            for (VkDescriptorPool pool : pools)
                vkDestroyDescriptorPool(m_device, pool, nullptr);

        for (uint32_t frame = 0; frame < k_framesInFlight; ++frame)
            deliverReadback(frame, false);

        destroyAllAttachments();
        destroyAllGraphBuffers();
    }

} //namespace vela::backend
