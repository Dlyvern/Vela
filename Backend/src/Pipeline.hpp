#ifndef VELA_BACKEND_PIPELINE_HPP
#define VELA_BACKEND_PIPELINE_HPP

#include "volk.h"

#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "Vela/Graphics/Vertex.hpp"
#include "Pass.hpp"

namespace vela::backend
{
    class LayoutCache
    {
    public:
        LayoutCache(VkDevice device);
        LayoutCache(const LayoutCache&) = delete;
        LayoutCache& operator=(const LayoutCache&) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> bindings);

        VkPipelineLayout getPipelineLayout(const std::vector<VkDescriptorSetLayout>& setLayouts,
            const std::vector<VkPushConstantRange>& pushConstantRanges);

        void clear();

        ~LayoutCache();

    private:
        VkDevice m_device{VK_NULL_HANDLE};

        std::unordered_map<uint64_t, VkDescriptorSetLayout> m_descriptorSetLayouts;
        std::unordered_map<uint64_t, VkPipelineLayout> m_pipelineLayouts;
    };

    enum class BlendMode : uint8_t
    {
        Opaque = 0,
    };

    uint64_t hashShaderCode(std::span<const uint32_t> vertexShader, std::span<const uint32_t> fragmentShader);

    struct PipelineDescription
    {
        std::span<const uint32_t> vertexShader;
        std::span<const uint32_t> fragmentShader;
        uint64_t shaderHash{0};
        graphics::VertexLayout vertexLayout;
        VkPipelineLayout layout;

        VkCullModeFlags cullMode{VK_CULL_MODE_NONE};
        VkFrontFace frontFace{VK_FRONT_FACE_CLOCKWISE};
        bool depthTest{true};
        bool depthWrite{true};
        VkCompareOp depthCompare{VK_COMPARE_OP_LESS};
        BlendMode blend{BlendMode::Opaque};
    };

    class PipelineCache
    {
    public:
        PipelineCache(VkDevice device);
        VkPipeline get(const PipelineDescription& description, const PassFormats& formats);
        void clear();
        ~PipelineCache();
    private:
        VkPipeline create(const PipelineDescription& description, const PassFormats& formats);

        uint64_t hashOf(const PipelineDescription& description, const PassFormats& formats);

        std::unordered_map<uint64_t, VkPipeline> m_pipelines;
        VkPipelineCache m_vkCache{VK_NULL_HANDLE};
        VkDevice m_device{VK_NULL_HANDLE};
    };
} //namespace vela::backend

#endif //VELA_BACKEND_PIPELINE_HPP