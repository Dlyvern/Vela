#include "Pipeline.hpp"

#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <vector>

namespace
{
    constexpr uint64_t k_fnvOffset = 1469598103934665603ull;
    constexpr uint64_t k_fnvPrime  = 1099511628211ull;

    uint64_t hashBytes(const void* data, size_t size, uint64_t seed)
    {
        const auto* bytes = static_cast<const uint8_t*>(data);
        uint64_t hash = seed;

        for (size_t i = 0; i < size; ++i)
        {
            hash ^= bytes[i];
            hash *= k_fnvPrime;
        }

        return hash;
    }

    template<typename T>
    uint64_t hashValue(const T& value, uint64_t seed)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        return hashBytes(&value, sizeof(T), seed);
    }

    VkShaderModule createShaderModule(VkDevice device, std::span<const uint32_t> code)
    {
        VkShaderModuleCreateInfo shaderModuleCI{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shaderModuleCI.codeSize = code.size_bytes();
        shaderModuleCI.pCode = code.data();
        
        VkShaderModule module;

        if (VkResult result = vkCreateShaderModule(device, &shaderModuleCI, nullptr, &module); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create shader module");

        return module;
    }

    VkFormat toVkFormat(vela::graphics::VertexAttributeFormat format)
    {
        switch (format)
        {
            case vela::graphics::VertexAttributeFormat::Float:  return VK_FORMAT_R32_SFLOAT;
            case vela::graphics::VertexAttributeFormat::Float2: return VK_FORMAT_R32G32_SFLOAT;
            case vela::graphics::VertexAttributeFormat::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
            case vela::graphics::VertexAttributeFormat::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case vela::graphics::VertexAttributeFormat::Unorm8x4: return VK_FORMAT_R8G8B8A8_UNORM;
        }

        throw std::runtime_error("Unknown vertex attribute format");
    }

    void applyBlendMode(VkPipelineColorBlendAttachmentState& attachment, vela::graphics::BlendMode mode)
    {
        switch (mode)
        {
            case vela::graphics::BlendMode::Opaque:
                attachment.blendEnable = VK_FALSE;
                return;
            case vela::graphics::BlendMode::Alpha:
                attachment.blendEnable = VK_TRUE;
                attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                attachment.colorBlendOp = VK_BLEND_OP_ADD;
                attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                attachment.alphaBlendOp = VK_BLEND_OP_ADD;
                return;
            case vela::graphics::BlendMode::Additive:
                attachment.blendEnable = VK_TRUE;
                attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                attachment.colorBlendOp = VK_BLEND_OP_ADD;
                attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                attachment.alphaBlendOp = VK_BLEND_OP_ADD;
                return;
        }

        throw std::runtime_error("Unknown blend mode");
    }
}

namespace vela::backend
{
    LayoutCache::LayoutCache(VkDevice device) : m_device(device)
    {
    }

    VkDescriptorSetLayout LayoutCache::getDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> bindings)
    {
        std::sort(bindings.begin(), bindings.end(),
            [](const VkDescriptorSetLayoutBinding& lhs, const VkDescriptorSetLayoutBinding& rhs)
            {
                return lhs.binding < rhs.binding;
            });

        uint64_t hash = k_fnvOffset;

        for (const VkDescriptorSetLayoutBinding& binding : bindings)
        {
            hash = hashValue(binding.binding, hash);
            hash = hashValue(binding.descriptorType, hash);
            hash = hashValue(binding.descriptorCount, hash);
            hash = hashValue(binding.stageFlags, hash);
        }

        if (auto it = m_descriptorSetLayouts.find(hash); it != m_descriptorSetLayouts.end())
            return it->second;

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetLayoutCI.pBindings = bindings.data();

        VkDescriptorSetLayout layout{VK_NULL_HANDLE};

        if (vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCI, nullptr, &layout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor set layout");

        m_descriptorSetLayouts.emplace(hash, layout);

        return layout;
    }

    VkPipelineLayout LayoutCache::getPipelineLayout(const std::vector<VkDescriptorSetLayout>& setLayouts,
        const std::vector<VkPushConstantRange>& pushConstantRanges)
    {
        uint64_t hash = k_fnvOffset;

        for (VkDescriptorSetLayout setLayout : setLayouts)
            hash = hashValue(setLayout, hash);

        for (const VkPushConstantRange& range : pushConstantRanges)
        {
            hash = hashValue(range.stageFlags, hash);
            hash = hashValue(range.offset, hash);
            hash = hashValue(range.size, hash);
        }

        if (auto it = m_pipelineLayouts.find(hash); it != m_pipelineLayouts.end())
            return it->second;

        VkPipelineLayoutCreateInfo pipelineLayoutCI{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        pipelineLayoutCI.pSetLayouts = setLayouts.data();
        pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
        pipelineLayoutCI.pPushConstantRanges = pushConstantRanges.data();

        VkPipelineLayout layout{VK_NULL_HANDLE};

        if (vkCreatePipelineLayout(m_device, &pipelineLayoutCI, nullptr, &layout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout");

        m_pipelineLayouts.emplace(hash, layout);

        return layout;
    }

    void LayoutCache::clear()
    {
        for (auto& [key, layout] : m_pipelineLayouts)
            vkDestroyPipelineLayout(m_device, layout, nullptr);

        for (auto& [key, layout] : m_descriptorSetLayouts)
            vkDestroyDescriptorSetLayout(m_device, layout, nullptr);

        m_pipelineLayouts.clear();
        m_descriptorSetLayouts.clear();
    }

    LayoutCache::~LayoutCache()
    {
        clear();
    }

    PipelineCache::PipelineCache(VkDevice device) : m_device(device)
    {
        VkPipelineCacheCreateInfo cacheCI{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};

        if (vkCreatePipelineCache(m_device, &cacheCI, nullptr, &m_vkCache) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline cache");

    }

    uint64_t hashShaderCode(std::span<const uint32_t> vertexShader, std::span<const uint32_t> fragmentShader)
    {
        const uint64_t hash = hashBytes(vertexShader.data(), vertexShader.size_bytes(), k_fnvOffset);
        return hashBytes(fragmentShader.data(), fragmentShader.size_bytes(), hash);
    }

    uint64_t PipelineCache::hashOf(const PipelineDescription& description, const PassFormats& formats)
    {
        uint64_t hash = k_fnvOffset;

        hash = hashValue(description.shaderHash, hash);

        hash = hashValue(description.vertexLayout.stride, hash);

        for (const auto& attribute : description.vertexLayout.attributes)
        {
            hash = hashValue(attribute.location, hash);
            hash = hashValue(attribute.format, hash);
            hash = hashValue(attribute.offset, hash);
        }

        hash = hashValue(description.vertexLayout.inputRate, hash);

        hash = hashValue(description.layout, hash);
        hash = hashValue(description.cullMode, hash);
        hash = hashValue(description.frontFace, hash);
        hash = hashValue(description.depthTest, hash);
        hash = hashValue(description.depthWrite, hash);
        hash = hashValue(description.depthCompare, hash);
        hash = hashValue(description.blend, hash);

        for (VkFormat format : formats.colorFormats)
            hash = hashValue(format, hash);

        return hashValue(formats.depthFormat, hash);
    }

    VkPipeline PipelineCache::create(const PipelineDescription& description, const PassFormats& formats)
    {
        auto vertShader = createShaderModule(m_device, description.vertexShader);
        auto fragShader = createShaderModule(m_device, description.fragmentShader);

        VkPipelineShaderStageCreateInfo vertStageCI{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        vertStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStageCI.module = vertShader;
        vertStageCI.pName = "main";

        VkPipelineShaderStageCreateInfo fragStageCI{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        fragStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStageCI.module = fragShader;
        fragStageCI.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStageCI[] = { vertStageCI, fragStageCI };

        const bool hasVertexInput = description.vertexLayout.stride > 0
            && !description.vertexLayout.attributes.empty();

        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = description.vertexLayout.stride;

        if(description.vertexLayout.inputRate == graphics::VertexInputRate::Vertex)
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        else if(description.vertexLayout.inputRate == graphics::VertexInputRate::Instance)
            binding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        else
            throw std::runtime_error("Unknown input rate");

        std::vector<VkVertexInputAttributeDescription> posAttr;
        posAttr.reserve(description.vertexLayout.attributes.size());

        for (const graphics::VertexAttribute& attribute : description.vertexLayout.attributes)
        {
            VkVertexInputAttributeDescription vkAttribute{};
            vkAttribute.binding = 0;
            vkAttribute.location = attribute.location;
            vkAttribute.format = toVkFormat(attribute.format);
            vkAttribute.offset = attribute.offset;

            posAttr.push_back(vkAttribute);
        }

        VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertexInput.vertexBindingDescriptionCount = hasVertexInput ? 1 : 0;
        vertexInput.pVertexBindingDescriptions = hasVertexInput ? &binding : nullptr;
        vertexInput.vertexAttributeDescriptionCount = hasVertexInput ? static_cast<uint32_t>(posAttr.size()) : 0;
        vertexInput.pVertexAttributeDescriptions = hasVertexInput ? posAttr.data() : nullptr;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.cullMode = description.cullMode;
        rasterizer.frontFace = description.frontFace;
        rasterizer.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        applyBlendMode(blendAttachment, description.blend);

        const std::vector<VkFormat>& colorFormats = formats.colorFormats;

        const std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(
            colorFormats.size(), blendAttachment);

        VkPipelineColorBlendStateCreateInfo colorBlend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        colorBlend.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
        colorBlend.pAttachments = blendAttachments.data();

        VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkPipelineRenderingCreateInfo renderingCI{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        renderingCI.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
        renderingCI.pColorAttachmentFormats = colorFormats.data();
        renderingCI.depthAttachmentFormat = formats.depthFormat;

        VkPipelineDepthStencilStateCreateInfo depthCI{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depthCI.depthTestEnable = description.depthTest ? VK_TRUE : VK_FALSE;
        depthCI.depthWriteEnable = description.depthWrite ? VK_TRUE : VK_FALSE;
        depthCI.depthCompareOp = description.depthCompare;

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
        pipelineCI.layout = description.layout;
        pipelineCI.pNext = &renderingCI;
        pipelineCI.pDepthStencilState = &depthCI;
        pipelineCI.renderPass = VK_NULL_HANDLE;
        pipelineCI.subpass = 0;

        VkPipeline pipeline{VK_NULL_HANDLE};

        const VkResult result = vkCreateGraphicsPipelines(m_device, m_vkCache, 1, &pipelineCI, nullptr, &pipeline);

        vkDestroyShaderModule(m_device, vertShader, nullptr);
        vkDestroyShaderModule(m_device, fragShader, nullptr);

        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline");

        return pipeline;
    }

    VkPipeline PipelineCache::get(const PipelineDescription& description, const PassFormats& formats)
    {
        const uint64_t key = hashOf(description, formats);

        if (auto it = m_pipelines.find(key); it != m_pipelines.end())
            return it->second;

        VkPipeline pipeline = create(description, formats);
        m_pipelines.emplace(key, pipeline);
        std::cout << "New pipeline was created with " << key << " key\n";

        return pipeline;
    }

    void PipelineCache::clear()
    {
        for (auto& [key, pipeline] : m_pipelines)
            vkDestroyPipeline(m_device, pipeline, nullptr);

        m_pipelines.clear();
    }

    PipelineCache::~PipelineCache()
    {
        clear();
        vkDestroyPipelineCache(m_device, m_vkCache, nullptr);
    }

} //namespace vela::backendcolorFormats