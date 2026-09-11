#ifndef VELA_BACKEND_MATERIAL_IMPL_HPP
#define VELA_BACKEND_MATERIAL_IMPL_HPP

#include "volk.h"
#include "vk_mem_alloc.h"

#include "Vela/Graphics/Material.hpp"

#include "Pipeline.hpp"
#include "DescriptorPool.hpp"

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::backend
{
    class DeletionQueue;
    
    class MaterialImpl
    {
    public:
        MaterialImpl(core::Context& context, const graphics::MaterialDescription& description);
        ~MaterialImpl();

        VkPipelineLayout getPipelineLayout() const;
        VkDescriptorSet getDescriptorSet() const;
        const PipelineDescription& getPipelineDescription() const;
        VkDescriptorSetLayout getDescriptorSetLayout() const;

        uint32_t getPushConstantSize() const;
        VkShaderStageFlags getPushConstantStages() const;

        Status setTexture(uint32_t slot, VkImageView imageView, VkSampler sampler);

        const graphics::MaterialDescription& getMaterialDescription() const;
    private:
        VmaAllocator m_allocator{VK_NULL_HANDLE};

        std::vector<uint32_t> m_vertexShader;
        std::vector<uint32_t> m_fragmentShader;

        graphics::MaterialDescription m_materialDescription;
        PipelineDescription m_pipelineDescription;

        DescriptorPool* m_descriptorPool{nullptr};
        DescriptorSetAllocation m_descriptorSet;
        VkDescriptorSetLayout m_descriptorSetLayout{VK_NULL_HANDLE};
        VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
        VkDevice m_device{VK_NULL_HANDLE};
        uint32_t m_textureCount{0};
        uint32_t m_pushConstantSize{0};
        VkShaderStageFlags m_pushConstantStages{0};
        DeletionQueue* m_deletionQueue{nullptr};
    };
} //namespace vela::backend

#endif //VELA_BACKEND_MATERIAL_IMPL_HPP