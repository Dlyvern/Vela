#include "ComputeProgramImpl.hpp"

#include "ContextImpl.hpp"
#include "DeletionQueue.hpp"

#include <algorithm>
#include <stdexcept>

namespace vela::backend
{
    ComputeProgramImpl::ComputeProgramImpl(core::Context& ctx, std::span<const uint32_t> computeShader)
        : m_device(ctx.impl()->getDevice())
    {
        if (computeShader.empty())
            throw std::runtime_error("Compute program requires compute SPIR-V");

        m_deletionQueue = &ctx.impl()->getDeletionQueue();
        m_bindings = reflectBindings(computeShader);

        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        layoutBindings.reserve(m_bindings.size());

        for (const ReflectedBinding& reflected : m_bindings)
        {
            if (reflected.set != 0)
                throw std::runtime_error("Compute program bindings must live in descriptor set 0");

            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding = reflected.binding;
            layoutBinding.descriptorCount = 1;
            layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

            switch (reflected.kind)
            {
                case DescriptorKind::StorageBuffer:
                    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    break;
                case DescriptorKind::UniformBuffer:
                    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    break;
                case DescriptorKind::CombinedImageSampler:
                    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    break;
            }

            layoutBindings.push_back(layoutBinding);
        }

        m_descriptorSetLayout = ctx.impl()->getLayoutCache().getDescriptorSetLayout(layoutBindings);

        if (!layoutBindings.empty())
        {
            m_descriptorPool = &ctx.impl()->getDescriptorPool();
            m_descriptorSet = m_descriptorPool->allocate(m_descriptorSetLayout);

            if (m_descriptorSet.set == VK_NULL_HANDLE)
                throw std::runtime_error("Failed to allocate compute descriptor set");
        }

        m_pushConstantSize = reflectPushConstantSize(computeShader);

        std::vector<VkPushConstantRange> pushConstantRanges;

        if (m_pushConstantSize > 0)
        {
            VkPushConstantRange pushConstantRange{};
            pushConstantRange.size = m_pushConstantSize;
            pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            pushConstantRange.offset = 0;

            pushConstantRanges.push_back(pushConstantRange);
        }

        m_pipelineLayout = ctx.impl()->getLayoutCache().getPipelineLayout({m_descriptorSetLayout}, pushConstantRanges);

        VkShaderModuleCreateInfo shaderModuleCI{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shaderModuleCI.codeSize = computeShader.size_bytes();
        shaderModuleCI.pCode = computeShader.data();

        VkShaderModule shaderModule{VK_NULL_HANDLE};

        if (vkCreateShaderModule(m_device, &shaderModuleCI, nullptr, &shaderModule) != VK_SUCCESS)
            throw std::runtime_error("Failed to create compute shader module");

        VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStage.module = shaderModule;
        shaderStage.pName = "main";

        VkComputePipelineCreateInfo pipelineCI{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        pipelineCI.stage = shaderStage;
        pipelineCI.layout = m_pipelineLayout;

        const VkResult result = vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_pipeline);

        vkDestroyShaderModule(m_device, shaderModule, nullptr);

        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to create compute pipeline");
    }

    Status ComputeProgramImpl::setStorageBuffer(uint32_t slot, VkBuffer buffer, VkDeviceSize size)
    {
        const auto binding = std::find_if(m_bindings.begin(), m_bindings.end(),
            [slot](const ReflectedBinding& reflected){ return reflected.binding == slot; });

        if (binding == m_bindings.end())
            return Error{ErrorCode::InvalidArgument, "Compute program has no binding at slot "
                + std::to_string(slot)};

        if (binding->kind != DescriptorKind::StorageBuffer)
            return Error{ErrorCode::InvalidArgument, "Binding at slot " + std::to_string(slot)
                + " is not a storage buffer"};

        if (const auto bound = m_boundStorageBuffers.find(slot);
            bound != m_boundStorageBuffers.end() && bound->second == buffer)
            return {};

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = size;

        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = m_descriptorSet.set;
        write.dstBinding = slot;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

        m_boundStorageBuffers[slot] = buffer;

        return {};
    }

    VkPipeline ComputeProgramImpl::getPipeline() const
    {
        return m_pipeline;
    }

    VkPipelineLayout ComputeProgramImpl::getPipelineLayout() const
    {
        return m_pipelineLayout;
    }

    VkDescriptorSet ComputeProgramImpl::getDescriptorSet() const
    {
        return m_descriptorSet.set;
    }

    uint32_t ComputeProgramImpl::getPushConstantSize() const
    {
        return m_pushConstantSize;
    }

    ComputeProgramImpl::~ComputeProgramImpl()
    {
        if (m_pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(m_device, m_pipeline, nullptr);

        if (m_deletionQueue != nullptr && m_descriptorSet.set != VK_NULL_HANDLE)
            m_deletionQueue->push({.descriptorSet = m_descriptorSet});
    }
} //namespace vela::backend
