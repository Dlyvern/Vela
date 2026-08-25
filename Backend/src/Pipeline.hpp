#ifndef VELA_BACKEND_PIPELINE_HPP
#define VELA_BACKEND_PIPELINE_HPP

#include "volk.h"

#include <cstdint>
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
        eOPAQUE = 0,
    };

    struct PipelineDescription
    {
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
        graphics::VertexLayout vertexLayout;
        VkPipelineLayout layout;

        VkCullModeFlags cullMode{VK_CULL_MODE_NONE};
        VkFrontFace frontFace{VK_FRONT_FACE_CLOCKWISE};
        bool depthTest{true};
        bool depthWrite{true};
        VkCompareOp depthCompare{VK_COMPARE_OP_LESS};
        BlendMode blend{BlendMode::eOPAQUE};
    };

    class PipelineCache
    {
    public:
        PipelineCache(VkDevice device);
        VkPipeline get(const PipelineDescription& description, const Pass& pass);
        void clear();
        ~PipelineCache();
    private:
        VkPipeline create(const PipelineDescription& description, const Pass& pass);

        uint64_t hashOf(const PipelineDescription& description, const Pass& pass);

        std::unordered_map<uint64_t, VkPipeline> m_pipelines;
        VkPipelineCache m_vkCache{VK_NULL_HANDLE};
        VkDevice m_device{VK_NULL_HANDLE};
    };
} //namespace vela::backend

#endif //VELA_BACKEND_PIPELINE_HPP