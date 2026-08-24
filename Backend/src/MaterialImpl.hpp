#ifndef VELA_BACKEND_MATERIAL_IMPL_HPP
#define VELA_BACKEND_MATERIAL_IMPL_HPP

#include <string>
#include <vector>

#include "volk.h"
#include "vk_mem_alloc.h"

#include "Vela/Graphics/Material.hpp"

#include "glm/mat4x4.hpp"

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::backend
{
    class RenderGraphImpl;

    class MaterialImpl
    {
    public:
        MaterialImpl(core::Context& context, RenderGraphImpl& renderGraph,
            const graphics::MaterialDescription& description);
        ~MaterialImpl();

        VkPipeline getPipeline() const;
        VkPipelineLayout getPipelineLayout() const;
        VkDescriptorSet getDescriptorSet() const;

        void setAlbedoTexture(VkImageView imageView, VkSampler sampler);
        void setMVP(const glm::mat4& view, const glm::mat4& projection);
    private:
        static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
        static VkFormat toVkFormat(graphics::VertexAttributeFormat format);

        VmaAllocator m_allocator{VK_NULL_HANDLE};
        VkBuffer m_mvpBuffer{VK_NULL_HANDLE};
        VmaAllocation m_mvpAllocation{VK_NULL_HANDLE};
        void* m_mvpMapped{nullptr};

        VkDescriptorPool m_descriptorPool{VK_NULL_HANDLE};
        VkDescriptorSet  m_descriptorSet{VK_NULL_HANDLE};
        VkDescriptorSetLayout m_descriptorSetLayout{VK_NULL_HANDLE};
        VkPipeline m_pipeline{VK_NULL_HANDLE};
        VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
        VkDevice m_device{VK_NULL_HANDLE};
    };
} //namespace vela::backend

#endif //VELA_BACKEND_MATERIAL_IMPL_HPP