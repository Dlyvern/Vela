#ifndef VELA_BACKEND_MATERIAL_IMPL_HPP
#define VELA_BACKEND_MATERIAL_IMPL_HPP

#include <string>
#include <vector>

#include "volk.h"
#include "vk_mem_alloc.h"

#include "Vela/Graphics/Material.hpp"

#include "Pipeline.hpp"

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

        VkPipelineLayout getPipelineLayout() const;
        VkDescriptorSet getDescriptorSet() const;
        const PipelineDescription& getPipelineDescription() const;
        VkDescriptorSetLayout getDescriptorSetLayout() const;

        void setAlbedoTexture(VkImageView imageView, VkSampler sampler);

        const graphics::MaterialDescription& getMaterialDescription() const;
    private:
        VmaAllocator m_allocator{VK_NULL_HANDLE};

        graphics::MaterialDescription m_materialDescription;
        PipelineDescription m_pipelineDescription;

        //TODO Every material should not create additional VkDescriptorPool
        VkDescriptorPool m_descriptorPool{VK_NULL_HANDLE};
        VkDescriptorSet  m_descriptorSet{VK_NULL_HANDLE};
        VkDescriptorSetLayout m_descriptorSetLayout{VK_NULL_HANDLE};
        VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
        VkDevice m_device{VK_NULL_HANDLE};
    };
} //namespace vela::backend

#endif //VELA_BACKEND_MATERIAL_IMPL_HPP