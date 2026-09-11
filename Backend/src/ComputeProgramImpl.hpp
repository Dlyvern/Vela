#ifndef VELA_BACKEND_COMPUTE_PROGRAM_IMPL_HPP
#define VELA_BACKEND_COMPUTE_PROGRAM_IMPL_HPP

#include "Vela/Core/Context.hpp"

#include "DescriptorPool.hpp"
#include "SpirvReflect.hpp"

#include "volk.h"

#include <span>
#include <unordered_map>
#include <vector>

namespace vela::backend
{
    class DeletionQueue;

    class ComputeProgramImpl
    {
    public:
        ComputeProgramImpl(core::Context& ctx, std::span<const uint32_t> computeShader);
        ~ComputeProgramImpl();

        VkPipeline getPipeline() const;
        VkPipelineLayout getPipelineLayout() const;
        VkDescriptorSet getDescriptorSet() const;

        uint32_t getPushConstantSize() const;

        Status setStorageBuffer(uint32_t slot, VkBuffer buffer, VkDeviceSize size);

    private:
        VkDevice m_device{VK_NULL_HANDLE};

        VkPipeline m_pipeline{VK_NULL_HANDLE};
        VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout m_descriptorSetLayout{VK_NULL_HANDLE};

        DescriptorPool* m_descriptorPool{nullptr};
        DescriptorSetAllocation m_descriptorSet;
        DeletionQueue* m_deletionQueue{nullptr};

        std::vector<ReflectedBinding> m_bindings;
        std::unordered_map<uint32_t, VkBuffer> m_boundStorageBuffers;

        uint32_t m_pushConstantSize{0};
    };
} //namespace vela::backend

#endif //VELA_BACKEND_COMPUTE_PROGRAM_IMPL_HPP
