#include "MaterialImpl.hpp"
#include "Vela/Utility/Resources.hpp"
#include "Vela/Core/Context.hpp"
#include "ContextImpl.hpp"
#include "Vela/Graphics/Vertex.hpp"
#include "RenderGraphImpl.hpp"

#include <stdexcept>

#include <cstring>
#include <vulkan/vulkan_core.h>

namespace vela::backend
{
    MaterialImpl::MaterialImpl(core::Context& context, RenderGraphImpl& renderGraph,
        const graphics::MaterialDescription& description) : m_device(context.impl()->getDevice()), m_materialDescription(description)
    {
        m_allocator = context.impl()->getAllocator();

        std::vector<VkDescriptorSetLayoutBinding> bindings(1);
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        m_descriptorSetLayout = renderGraph.getLayoutCache().getDescriptorSetLayout(bindings);

        std::vector<VkDescriptorPoolSize> poolSizes(1);
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = 1;

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

        VkPushConstantRange modelPushConstant{};
        modelPushConstant.size = sizeof(glm::mat4);
        modelPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        modelPushConstant.offset = 0;

        m_pipelineLayout = renderGraph.getLayoutCache().getPipelineLayout(
            {renderGraph.getPerViewDescriptorSetLayout(), m_descriptorSetLayout}, {modelPushConstant});

        m_pipelineDescription.vertexShaderPath = m_materialDescription.vertexShaderPath;
        m_pipelineDescription.fragmentShaderPath = m_materialDescription.fragmentShaderPath;
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

    void MaterialImpl::setAlbedoTexture(VkImageView imageView, VkSampler sampler)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView;
        imageInfo.sampler = sampler;

        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = m_descriptorSet;
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
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