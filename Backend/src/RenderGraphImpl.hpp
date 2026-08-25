#ifndef VELA_BACKEND_RENDER_GRAPH_IMPL_HPP
#define VELA_BACKEND_RENDER_GRAPH_IMPL_HPP

#include "volk.h"

#include "PresentPass.hpp"
#include "Pipeline.hpp"

#include "vk_mem_alloc.h"

#include <memory>

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <array>
#include "glm/mat4x4.hpp"

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::graphics
{
    class Material;
    class Mesh;
} //namespace vela::graphics

//TODO Create gbuffer render graph pass and depth render graph pass.
//TODO Make those render graph passes not primary(Make cmake flag to build them maybe some developers don't want my broken shaders and shitty code)

namespace vela::backend
{
    class RenderGraphImpl
    {
    public:
        RenderGraphImpl(core::Context& context);

        void beginFrame();
        void endFrame();

        void beginPresentPass(float r, float g, float b, float a);

        const std::vector<VkFormat>& getColorFormats() const;

        LayoutCache& getLayoutCache();

        VkDescriptorSetLayout getPerViewDescriptorSetLayout() const;

        void draw(const graphics::Mesh& mesh, const graphics::Material& material, const glm::mat4& model);

        void endRenderPass();

        void updatePerViewDescriptors(const glm::mat4& view, const glm::mat4& projection);

        ~RenderGraphImpl();
    private:
        // How many frames the CPU may run ahead of the GPU. One would mean the
        // CPU blocks on the previous frame before it can record the next one,
        // so every GPU/present stall lands directly in the frame time
        static constexpr uint32_t k_framesInFlight{2};

        void buildRenderGraphPasses();
        void allocateRenderGraphPassOutputs(const Pass& pass);

        PassContext makePassContext() const;

        void createSwapchainSyncObjects();
        void destroySwapchainSyncObjects();
        void recreateSwapchainResources();
        void destroyAllAttachments();

        core::Context& m_context;
 
        std::unique_ptr<PresentPass> m_presentPass;

        // Per frame-in-flight: waited on by the submit that renders into the
        // image this frame acquired
        std::vector<VkSemaphore> m_imageAvailable;
        std::vector<VkFence> m_inFlightFences;
        std::vector<VkCommandBuffer> m_commandBuffers;

        std::vector<VkSemaphore> m_renderFinished;

        VkDevice m_device{VK_NULL_HANDLE};
        VkQueue m_graphicsQueue{VK_NULL_HANDLE};
        LayoutCache m_layoutCache;
        PipelineCache m_pipelineCache;

        bool m_isFrameValid{true};

        std::unordered_map<std::string, Attachment> m_attachments;

        uint32_t m_frameIndex{0};
        uint32_t m_currentImageIndex{0};
        VkCommandBuffer m_currentCommandBuffer{VK_NULL_HANDLE};


        VkDescriptorSetLayout m_perViewDescriptorSetLayout{VK_NULL_HANDLE};
        VkDescriptorPool m_descriptorPool{VK_NULL_HANDLE};
        std::array<VkDescriptorSet, k_framesInFlight> m_perViewDescriptorSets;
        std::array<VkBuffer, k_framesInFlight> m_perViewBuffer{VK_NULL_HANDLE};
        std::array<VmaAllocation, k_framesInFlight> m_perViewBufferAllocation{VK_NULL_HANDLE};
        std::array<void*, k_framesInFlight> m_perViewMapped{nullptr};

    };
} //namespace vela::backend


#endif //VELA_BACKEND_RENDER_GRAPH_IMPL_HPP
