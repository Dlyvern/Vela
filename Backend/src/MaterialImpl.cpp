#include "MaterialImpl.hpp"
#include "Vela/Core/Context.hpp"
#include "ContextImpl.hpp"
#include "SpirvReflect.hpp"
#include "DynamicBufferImpl.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>

#include <vulkan/vulkan_core.h>

#include "Vela/Math/Matrix.hpp"

#include "Formats.hpp"

namespace vela::backend
{
    namespace
    {
        constexpr uint32_t k_materialSet = 1;
    }

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

        m_allocator = context.impl()->getAllocator();

        std::map<uint32_t, VkDescriptorSetLayoutBinding> bindingsBySlot;

        auto collectBindings = [this, &bindingsBySlot](std::span<const uint32_t> shader, VkShaderStageFlags stage)
        {
            for (const ReflectedBinding& reflected : reflectBindings(shader))
            {
                if (reflected.set != k_materialSet)
                    continue;

                VkDescriptorSetLayoutBinding& binding = bindingsBySlot[reflected.binding];
                binding.binding = reflected.binding;
                binding.descriptorCount = 1;
                binding.stageFlags |= stage;

                switch (reflected.kind)
                {
                    case DescriptorKind::StorageBuffer:
                        binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        break;
                    case DescriptorKind::UniformBuffer:
                        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        break;
                    case DescriptorKind::CombinedImageSampler:
                        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        break;
                }

                m_bindingKinds[reflected.binding] = reflected.kind;
            }
        };

        collectBindings(m_vertexShader, VK_SHADER_STAGE_VERTEX_BIT);
        collectBindings(m_fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT);

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(bindingsBySlot.size());

        for (const auto& [slot, binding] : bindingsBySlot)
            bindings.push_back(binding);

        m_descriptorSetLayout = context.impl()->getLayoutCache().getDescriptorSetLayout(bindings);
        m_deletionQueue = &context.impl()->getDeletionQueue();

        if(!bindings.empty())
        {
            m_descriptorPool = &context.impl()->getDescriptorPool();
            for (DescriptorSetAllocation& descriptorSet : m_descriptorSets)
                descriptorSet = m_descriptorPool->allocate(m_descriptorSetLayout);
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
            {context.impl()->getPerViewDescriptorSetLayout(), m_descriptorSetLayout,
             context.impl()->getPassInputDescriptorSetLayout()}, pushConstantRanges);

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
        const auto kind = m_bindingKinds.find(slot);

        if (kind == m_bindingKinds.end() || kind->second != DescriptorKind::CombinedImageSampler)
            return Error{ErrorCode::InvalidArgument, "Material has no texture at slot " + std::to_string(slot)};

        Binding binding{};
        binding.imageView = imageView;
        binding.sampler = sampler;

        m_bindings[slot] = binding;

        return {};
    }

    Status MaterialImpl::setStorageBuffer(uint32_t slot, VkBuffer buffer, VkDeviceSize size)
    {
        const auto kind = m_bindingKinds.find(slot);

        if (kind == m_bindingKinds.end() || kind->second != DescriptorKind::StorageBuffer)
            return Error{ErrorCode::InvalidArgument, "Material has no storage buffer at slot "
                + std::to_string(slot)};

        Binding binding{};
        binding.buffer = buffer;
        binding.size = size;

        m_bindings[slot] = binding;

        return {};
    }

    Status MaterialImpl::setStorageBuffer(uint32_t slot, const DynamicBufferImpl& buffer)
    {
        const auto kind = m_bindingKinds.find(slot);

        if (kind == m_bindingKinds.end() || kind->second != DescriptorKind::StorageBuffer)
            return Error{ErrorCode::InvalidArgument, "Material has no storage buffer at slot "
                + std::to_string(slot)};

        Binding binding{};
        binding.dynamicBuffer = &buffer;

        m_bindings[slot] = binding;

        return {};
    }

    MaterialImpl::Binding MaterialImpl::resolve(const Binding& binding) const
    {
        if (binding.dynamicBuffer == nullptr)
            return binding;

        Binding resolved = binding;
        resolved.buffer = binding.dynamicBuffer->getBuffer();
        resolved.size = binding.dynamicBuffer->getSize();

        return resolved;
    }

    VkDescriptorSet MaterialImpl::getDescriptorSet(uint32_t frameIndex)
    {
        const VkDescriptorSet set = m_descriptorSets[frameIndex].set;

        if (set == VK_NULL_HANDLE)
            return VK_NULL_HANDLE;

        std::unordered_map<uint32_t, Binding>& written = m_writtenBindings[frameIndex];

        std::vector<VkDescriptorImageInfo> imageInfos;
        std::vector<VkDescriptorBufferInfo> bufferInfos;
        std::vector<VkWriteDescriptorSet> writes;

        imageInfos.reserve(m_bindings.size());
        bufferInfos.reserve(m_bindings.size());

        for (const auto& [slot, desired] : m_bindings)
        {
            const Binding binding = resolve(desired);

            if (const auto it = written.find(slot); it != written.end() && it->second == binding)
                continue;

            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet = set;
            write.dstBinding = slot;
            write.descriptorCount = 1;

            if (binding.imageView != VK_NULL_HANDLE)
            {
                VkDescriptorImageInfo& imageInfo = imageInfos.emplace_back();
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = binding.imageView;
                imageInfo.sampler = binding.sampler;

                write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                write.pImageInfo = &imageInfo;
            }
            else
            {
                if (binding.buffer == VK_NULL_HANDLE)
                    continue;

                VkDescriptorBufferInfo& bufferInfo = bufferInfos.emplace_back();
                bufferInfo.buffer = binding.buffer;
                bufferInfo.offset = 0;
                bufferInfo.range = binding.size;

                write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                write.pBufferInfo = &bufferInfo;
            }

            writes.push_back(write);
            written[slot] = binding;
        }

        if (!writes.empty())
            vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

        return set;
    }

    const graphics::MaterialDescription& MaterialImpl::getMaterialDescription() const
    {
        return m_materialDescription;
    }

    MaterialImpl::~MaterialImpl()
    {
        if(!m_deletionQueue)
            return;

        for (const DescriptorSetAllocation& descriptorSet : m_descriptorSets)
            if (descriptorSet.set != VK_NULL_HANDLE)
                m_deletionQueue->push({.descriptorSet = descriptorSet});
    }

    VkPipelineLayout MaterialImpl::getPipelineLayout() const
    {
        return m_pipelineLayout;
    }
} //namespace vela::backend