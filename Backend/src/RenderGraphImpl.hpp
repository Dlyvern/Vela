#ifndef VELA_BACKEND_RENDER_GRAPH_IMPL_HPP
#define VELA_BACKEND_RENDER_GRAPH_IMPL_HPP

#include "volk.h"

#include "Pipeline.hpp"
#include "Pass.hpp"
#include "PresentPass.hpp"

#include "vk_mem_alloc.h"

#include <memory>

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <array>

#include "Vela/Math/Matrix.hpp"

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
//TODO make RenderGraph works as an auto mode where you just implement RGPs, and based on their dependencies it calculates 
// what RGP should run after or before, like You set output of ShadowRGP is depth and then LightRGP inputs depth and in that way RGP understand the 
//execution order. Or you can just manually type RGP::beginPass("Shadow"), draw something, RGP::endPass(), and then RGP::beginPass("Light") 

namespace vela::backend
{
    class RenderGraphImpl
    {
    public:
        RenderGraphImpl(core::Context& context);

        void beginFrame();
        void endFrame();

        void beginPass(const std::string& renderGraphPassName);

        void draw(const graphics::Mesh& mesh, const graphics::Material& material, const math::Mat4& model);

        void endPass();

        void updatePerViewDescriptors(const math::Mat4& view, const math::Mat4& projection);

        ~RenderGraphImpl();
    private:
        struct RegisteredPass
        {
            std::unique_ptr<Pass> pass;
            PassDeclaration declaration;
            PassFormats formats;
        };

        // Execution order is the vector order; the map is lookup by name only.
        std::vector<RegisteredPass> m_renderGraphPasses;
        std::unordered_map<std::string, size_t> m_renderGraphPassIndices;
        std::unordered_map<std::string, VkImageLayout> m_attachmentLayouts;

        RegisteredPass* m_currentRenderGraphPass{nullptr};
        bool m_currentPassOpenedRendering{false};

        // How many frames the CPU may run ahead of the GPU. One would mean the
        // CPU blocks on the previous frame before it can record the next one,
        // so every GPU/present stall lands directly in the frame time
        static constexpr uint32_t k_framesInFlight{2};

        Pass& addRenderGraphPass(const std::string& name, std::unique_ptr<Pass> pass);

        void allocateAllRenderGraphPassOutputs();
        void allocateRenderGraphPassOutputs(const PassDeclaration& declaration,
            const std::unordered_map<std::string, VkImageUsageFlags>& usages);
        Attachment allocateAttachment(const AttachmentOutput& attachmentOutput, VkImageUsageFlags usage, bool isDepth);
        VkExtent2D passExtent(const PassDeclaration& declaration) const;

        void createSwapchainSyncObjects();
        void destroySwapchainSyncObjects();
        void recreateSwapchainResources();
        void destroyAllAttachments();
        void resetAllAttachmentLayouts();

        core::Context& m_context;
 
        // Per frame-in-flight: waited on by the submit that renders into the
        // image this frame acquired
        std::vector<VkSemaphore> m_imageAvailable;
        std::vector<VkFence> m_inFlightFences;
        std::vector<VkCommandBuffer> m_commandBuffers;

        std::vector<VkSemaphore> m_renderFinished;

        VkDevice m_device{VK_NULL_HANDLE};
        VkQueue m_graphicsQueue{VK_NULL_HANDLE};
        PipelineCache m_pipelineCache;

        bool m_isFrameValid{true};

        std::unordered_map<std::string, Attachment> m_attachments;

        uint32_t m_frameIndex{0};
        uint32_t m_currentImageIndex{0};
        VkCommandBuffer m_currentCommandBuffer{VK_NULL_HANDLE};

        VkDescriptorPool m_descriptorPool{VK_NULL_HANDLE};
        std::array<VkDescriptorSet, k_framesInFlight> m_perViewDescriptorSets;
        std::array<VkBuffer, k_framesInFlight> m_perViewBuffer{VK_NULL_HANDLE};
        std::array<VmaAllocation, k_framesInFlight> m_perViewBufferAllocation{VK_NULL_HANDLE};
        std::array<void*, k_framesInFlight> m_perViewMapped{nullptr};

    };
} //namespace vela::backend


#endif //VELA_BACKEND_RENDER_GRAPH_IMPL_HPP
