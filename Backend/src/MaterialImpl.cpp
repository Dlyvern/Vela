#include "MaterialImpl.hpp"
#include "Vela/Core/Context.hpp"
#include "ContextImpl.hpp"
#include "SpirvReflect.hpp"

#include <algorithm>
#include <stdexcept>

#include <vulkan/vulkan_core.h>

#include "Vela/Math/Matrix.hpp"

#include "Formats.hpp"

namespace vela::backend
{
    MaterialImpl::MaterialImpl(core::Context& context,
        const graphics::MaterialDescription& description) : m_materialDescription(description),
        m_device(context.impl()->getDevice())
    {
        if (description.vertexShader.empty() || description.fragmentShader.empty())
            throw std::runtime_error("Material requires vertex and fragment SPIR-V");

        m_vertexShader.assign(description.vertexShader.begin(), description.vertexShader.end());
        m_fragmentShader.assign(description.fragmentShader.begin(), description.fragmentShader.end());

        m_materialDescription.vertexShader = m_vertexShader;
        m_materialDescription.fragmentShader = m_fragmentShader;

        m_textureCount = reflectTextureCount(m_vertexShader, m_fragmentShader);

        m_allocator = context.impl()->getAllocator();

        std::vector<VkDescriptorSetLayoutBinding> bindings(m_textureCount);

        for(uint32_t index = 0; index < m_textureCount; ++index)
        {
            auto& binding = bindings[index];
            binding.binding = index;
            binding.descriptorCount = 1;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        m_descriptorSetLayout = context.impl()->getLayoutCache().getDescriptorSetLayout(bindings);
        m_deletionQueue = &context.impl()->getDeletionQueue();

        if(m_textureCount > 0)
        {
            m_descriptorPool = &context.impl()->getDescriptorPool();
            m_descriptorSet = m_descriptorPool->allocate(m_descriptorSetLayout);
        }

        const uint32_t vertexPushConstantSize = reflectPushConstantSize(m_vertexShader);
        const uint32_t fragmentPushConstantSize = reflectPushConstantSize(m_fragmentShader);

        m_pushConstantSize = std::max(vertexPushConstantSize, fragmentPushConstantSize);
        m_pushConstantStages = 0;

        if (vertexPushConstantSize > 0)
            m_pushConstantStages |= VK_SHADER_STAGE_VERTEX_BIT;

        if (fragmentPushConstantSize > 0)
            m_pushConstantStages |= VK_SHADER_STAGE_FRAGMENT_BIT;

        std::vector<VkPushConstantRange> pushConstantRanges;

        if (m_pushConstantSize > 0)
        {
            VkPushConstantRange pushConstantRange{};
            pushConstantRange.size = m_pushConstantSize;
            pushConstantRange.stageFlags = m_pushConstantStages;
            pushConstantRange.offset = 0;

            pushConstantRanges.push_back(pushConstantRange);
        }

        m_pipelineLayout = context.impl()->getLayoutCache().getPipelineLayout(
            {context.impl()->getPerViewDescriptorSetLayout(), m_descriptorSetLayout}, pushConstantRanges);

        m_pipelineDescription.vertexShader = m_vertexShader;
        m_pipelineDescription.fragmentShader = m_fragmentShader;
        m_pipelineDescription.shaderHash = hashShaderCode(m_vertexShader, m_fragmentShader);
        m_pipelineDescription.vertexLayout = m_materialDescription.vertexLayout;
        m_pipelineDescription.layout = m_pipelineLayout;

        m_pipelineDescription.cullMode = toVkCullMode(m_materialDescription.renderState.cull);
        m_pipelineDescription.frontFace = toVkFrontFace(m_materialDescription.renderState.frontFace);
        m_pipelineDescription.depthTest = m_materialDescription.renderState.depthTest;
        m_pipelineDescription.depthWrite = m_materialDescription.renderState.depthWrite;
        m_pipelineDescription.depthCompare = toVkCompare(m_materialDescription.renderState.depthCompare);
        m_pipelineDescription.blend = m_materialDescription.renderState.blend;
    }

    uint32_t MaterialImpl::getPushConstantSize() const
    {
        return m_pushConstantSize;
    }

    VkShaderStageFlags MaterialImpl::getPushConstantStages() const
    {
        return m_pushConstantStages;
    }

    VkDescriptorSetLayout MaterialImpl::getDescriptorSetLayout() const
    {
        return m_descriptorSetLayout;
    }

    const PipelineDescription& MaterialImpl::getPipelineDescription() const
    {
        return m_pipelineDescription;
    }

    Status MaterialImpl::setTexture(uint32_t slot, VkImageView imageView, VkSampler sampler)
    {
        if(slot >= m_textureCount)
            return Error{ErrorCode::InvalidArgument, "Texture slot " + std::to_string(slot)
                + " is out of range for a material declaring " + std::to_string(m_textureCount) + " textures"};

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView;
        imageInfo.sampler = sampler;

        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = m_descriptorSet.set;
        write.dstBinding = slot;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

        return {};
    }

    const graphics::MaterialDescription& MaterialImpl::getMaterialDescription() const
    {
        return m_materialDescription;
    }

    MaterialImpl::~MaterialImpl()
    {
        if(m_deletionQueue)
            m_deletionQueue->push({.descriptorSet = m_descriptorSet});
    }

    VkDescriptorSet MaterialImpl::getDescriptorSet() const
    {
        return m_descriptorSet.set;
    }

    VkPipelineLayout MaterialImpl::getPipelineLayout() const
    {
        return m_pipelineLayout;
    }
} //namespace vela::backend