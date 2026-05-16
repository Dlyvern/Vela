#include "MaterialImpl.hpp"
#include "Vela/Utility/Resources.hpp"
#include "Vela/Core/Context.hpp"
#include "ContextImpl.hpp"
#include "Vela/Graphics/Vertex.hpp"

#include <cstring>

struct MVP { glm::mat4 model, view, projection; };

namespace vela::backend
{
    MaterialImpl::MaterialImpl(core::Context& context, const std::string& vertShaderPath, const std::string& fragShaderPath) : m_device(context.impl()->getDevice())
    {
        m_allocator = context.impl()->getAllocator();

        std::vector<VkDescriptorSetLayoutBinding> bindings(2);
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetLayoutCI.pBindings = bindings.data();

        if(VkResult result = vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCI, nullptr, &m_descriptorSetLayout);
            result != VK_SUCCESS)
                throw std::runtime_error("Failed to create descriptor set layout");

        std::vector<VkDescriptorPoolSize> poolSizes(2);
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = 1;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = 1;

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

        VkDeviceSize imageSize = sizeof(MVP);

        VkBufferCreateInfo mvpBufferCI{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        mvpBufferCI.size = imageSize;
        mvpBufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        mvpBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo mvpBufferAllocationCI{};
        mvpBufferAllocationCI.usage = VMA_MEMORY_USAGE_AUTO;
        mvpBufferAllocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                            | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo info{};

        if(VkResult result = vmaCreateBuffer(context.impl()->getAllocator(), &mvpBufferCI, &mvpBufferAllocationCI, 
            &m_mvpBuffer, &m_mvpAllocation, &info); result != VK_SUCCESS)
                throw std::runtime_error("Faild to allocate buffer");

        m_mvpMapped = info.pMappedData;

        VkDescriptorBufferInfo bufInfo{};
        bufInfo.buffer = m_mvpBuffer;
        bufInfo.offset = 0;
        bufInfo.range = sizeof(MVP);

        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = m_descriptorSet;
        write.dstBinding = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufInfo;

        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

        auto vertCode = utilities::resources::readFileShader(vertShaderPath);
        auto fragCode = utilities::resources::readFileShader(fragShaderPath);
        
        auto vertShader = createShaderModule(m_device, vertCode);
        auto fragShader = createShaderModule(m_device, fragCode);
        
        VkPipelineLayoutCreateInfo pipelineLayoutCI{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pipelineLayoutCI.setLayoutCount = 1;
        pipelineLayoutCI.pSetLayouts = &m_descriptorSetLayout;
        pipelineLayoutCI.pPushConstantRanges = 0;
        pipelineLayoutCI.pPushConstantRanges = nullptr;

        if(VkResult result = vkCreatePipelineLayout(m_device, &pipelineLayoutCI, nullptr, &m_pipelineLayout); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout");

        VkPipelineShaderStageCreateInfo vertStageCI{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        vertStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStageCI.module = vertShader;
        vertStageCI.pName = "main";

        VkPipelineShaderStageCreateInfo fragStageCI{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        fragStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStageCI.module = fragShader;
        fragStageCI.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStageCI[] = { vertStageCI, fragStageCI };

        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(vela::graphics::Vertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> posAttr(2);
        posAttr[0].binding = 0;
        posAttr[0].location = 0;                  
        posAttr[0].format = VK_FORMAT_R32G32_SFLOAT;
        posAttr[0].offset = offsetof(graphics::Vertex, position);

        posAttr[1].binding = 0;
        posAttr[1].location = 1;                  
        posAttr[1].format = VK_FORMAT_R32G32_SFLOAT;
        posAttr[1].offset = offsetof(graphics::Vertex, uv);

        VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &binding;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(posAttr.size());
        vertexInput.pVertexAttributeDescriptions = posAttr.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &blendAttachment;

        VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkGraphicsPipelineCreateInfo pipelineCI{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipelineCI.stageCount = 2;
        pipelineCI.pStages = shaderStageCI;
        pipelineCI.pVertexInputState = &vertexInput;
        pipelineCI.pInputAssemblyState = &inputAssemblyCI;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pRasterizationState = &rasterizer;
        pipelineCI.pMultisampleState = &multisample;
        pipelineCI.pColorBlendState = &colorBlend;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.layout = m_pipelineLayout;
        pipelineCI.renderPass = context.impl()->getRenderPass();
        pipelineCI.subpass = 0;

        if (VkResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_pipeline); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline");

        vkDestroyShaderModule(m_device, vertShader, nullptr);
        vkDestroyShaderModule(m_device, fragShader, nullptr);
    }

    void MaterialImpl::setMVP(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
    {
        MVP mvp{ model, view, projection };
        std::memcpy(m_mvpMapped, &mvp, sizeof(MVP));
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

    MaterialImpl::~MaterialImpl()
    {
        vmaDestroyBuffer(m_allocator, m_mvpBuffer, m_mvpAllocation);
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
    }

    VkPipeline MaterialImpl::getPipeline() const
    {
        return m_pipeline;
    }

    VkDescriptorSet MaterialImpl::getDescriptorSet() const
    {
        return m_descriptorSet;
    }

    VkPipelineLayout MaterialImpl::getPipelineLayout() const
    {
        return m_pipelineLayout;
    }

    VkShaderModule MaterialImpl::createShaderModule(VkDevice device, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo shaderModuleCI{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shaderModuleCI.codeSize = static_cast<uint32_t>(code.size());
        shaderModuleCI.pCode = reinterpret_cast<const uint32_t*>(code.data());
        
        VkShaderModule module;

        if (VkResult result = vkCreateShaderModule(device, &shaderModuleCI, nullptr, &module); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create shader module");

        return module;
    }
} //namespace vela::backend