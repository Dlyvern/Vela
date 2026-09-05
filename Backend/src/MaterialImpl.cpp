#include "MaterialImpl.hpp"
#include "Vela/Core/Context.hpp"
#include "ContextImpl.hpp"

#include <stdexcept>

#include <vulkan/vulkan_core.h>

#include "Vela/Math/Matrix.hpp"

namespace vela::backend
{
    MaterialImpl::MaterialImpl(core::Context& context,
        const graphics::MaterialDescription& description) : m_materialDescription(description),
        m_device(context.impl()->getDevice()), m_textureCount(description.textureCount)
    {
        if (description.vertexShader.empty() || description.fragmentShader.empty())
            throw std::runtime_error("Material requires vertex and fragment SPIR-V");

        m_vertexShader.assign(description.vertexShader.begin(), description.vertexShader.end());
        m_fragmentShader.assign(description.fragmentShader.begin(), description.fragmentShader.end());

        m_materialDescription.vertexShader = m_vertexShader;
        m_materialDescription.fragmentShader = m_fragmentShader;

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

        if(m_textureCount > 0)
        {
            std::vector<VkDescriptorPoolSize> poolSizes(1);
            poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            poolSizes[0].descriptorCount = m_textureCount;

            VkDescriptorPoolCreateInfo descriptorPoolCI{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
            descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            descriptorPoolCI.pPoolSizes = poolSizes.data();
            descriptorPoolCI.maxSets = 1;

            if(VkResult result = vkCreateDescriptorPool(m_device, &descriptorPoolCI, nullptr, &m_descriptorPool); result != VK_SUCCESS)
                throw std::runtime_error("Failed to create descriptor pool");

            VkDescriptorSetAllocateInfo descriptorSetAI{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            descriptorSetAI.descriptorPool = m_descriptorPool;
            descriptorSetAI.descriptorSetCount = 1;
            descriptorSetAI.pSetLayouts = &m_descriptorSetLayout;

            if(VkResult result = vkAllocateDescriptorSets(m_device, &descriptorSetAI, &m_descriptorSet); result != VK_SUCCESS)
                throw std::runtime_error("Failed to allocate descriptor sets");
        }

        VkPushConstantRange modelPushConstant{};
        modelPushConstant.size = sizeof(math::Mat4);
        modelPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        modelPushConstant.offset = 0;

        m_pipelineLayout = context.impl()->getLayoutCache().getPipelineLayout(
            {context.impl()->getPerViewDescriptorSetLayout(), m_descriptorSetLayout}, {modelPushConstant});

        m_pipelineDescription.vertexShader = m_vertexShader;
        m_pipelineDescription.fragmentShader = m_fragmentShader;
        m_pipelineDescription.shaderHash = hashShaderCode(m_vertexShader, m_fragmentShader);
        m_pipelineDescription.vertexLayout = m_materialDescription.vertexLayout;
        m_pipelineDescription.layout = m_pipelineLayout;
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
        write.dstSet = m_descriptorSet;
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
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    }

    VkDescriptorSet MaterialImpl::getDescriptorSet() const
    {
        return m_descriptorSet;
    }

    VkPipelineLayout MaterialImpl::getPipelineLayout() const
    {
        return m_pipelineLayout;
    }
} //namespace vela::backend