#ifndef VELA_BACKEND_MATERIAL_IMPL_HPP
#define VELA_BACKEND_MATERIAL_IMPL_HPP

#include "volk.h"
#include "vk_mem_alloc.h"

#include "Vela/Graphics/Material.hpp"

#include "Pipeline.hpp"
#include "DescriptorPool.hpp"
#include "SpirvReflect.hpp"
#include "FrameConstants.hpp"

#include <array>
#include <unordered_map>

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::backend
{
    class DeletionQueue;
    class DynamicBufferImpl;
    
    class MaterialImpl
    {
    public:
        MaterialImpl(core::Context& context, const graphics::MaterialDescription& description);
        ~MaterialImpl();

        VkPipelineLayout getPipelineLayout() const;

        /*
            One set per frame in flight. Bindings changed by setTexture/setStorageBuffer are written into a frame's
            set the first time that frame asks for it, so a set the GPU may still be reading is never touched
        */
        VkDescriptorSet getDescriptorSet(uint32_t frameIndex);
        const PipelineDescription& getPipelineDescription() const;
        VkDescriptorSetLayout getDescriptorSetLayout() const;

        uint32_t getPushConstantSize() const;
        VkShaderStageFlags getPushConstantStages() const;

        Status setTexture(uint32_t slot, VkImageView imageView, VkSampler sampler);
        Status setStorageBuffer(uint32_t slot, VkBuffer buffer, VkDeviceSize size);

        // The buffer is resolved per frame, since a DynamicBuffer rotates its storage between frames in flight
        Status setStorageBuffer(uint32_t slot, const DynamicBufferImpl& buffer);

        const graphics::MaterialDescription& getMaterialDescription() const;
    private:
        VmaAllocator m_allocator{VK_NULL_HANDLE};

        std::vector<uint32_t> m_vertexShader;
        std::vector<uint32_t> m_fragmentShader;

        graphics::MaterialDescription m_materialDescription;
        PipelineDescription m_pipelineDescription;

        static constexpr uint32_t k_framesInFlight = FrameConstants::k_framesInFlight;

        struct Binding
        {
            VkImageView imageView{VK_NULL_HANDLE};
            VkSampler sampler{VK_NULL_HANDLE};
            VkBuffer buffer{VK_NULL_HANDLE};
            VkDeviceSize size{0};
            const DynamicBufferImpl* dynamicBuffer{nullptr};

            bool operator==(const Binding&) const = default;
        };

        [[nodiscard]] Binding resolve(const Binding& binding) const;

        DescriptorPool* m_descriptorPool{nullptr};
        std::array<DescriptorSetAllocation, k_framesInFlight> m_descriptorSets{};

        std::unordered_map<uint32_t, Binding> m_bindings;
        std::array<std::unordered_map<uint32_t, Binding>, k_framesInFlight> m_writtenBindings;
        VkDescriptorSetLayout m_descriptorSetLayout{VK_NULL_HANDLE};
        VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
        VkDevice m_device{VK_NULL_HANDLE};
        std::unordered_map<uint32_t, DescriptorKind> m_bindingKinds;
        uint32_t m_pushConstantSize{0};
        VkShaderStageFlags m_pushConstantStages{0};
        DeletionQueue* m_deletionQueue{nullptr};
    };
} //namespace vela::backend

#endif //VELA_BACKEND_MATERIAL_IMPL_HPP