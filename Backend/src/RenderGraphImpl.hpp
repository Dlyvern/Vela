#ifndef VELA_BACKEND_RENDER_GRAPH_IMPL_HPP
#define VELA_BACKEND_RENDER_GRAPH_IMPL_HPP

#include "volk.h"
#include "vk_mem_alloc.h"

#include "Pipeline.hpp"
#include "Pass.hpp"
#include "FrameConstants.hpp"
#include "ContextImpl.hpp"

#include "Vela/Result.hpp"
#include "Vela/Math/Matrix.hpp"
#include "Vela/Math/Vector.hpp"
#include "Vela/Graphics/Pass.hpp"
#include "Vela/Graphics/FrameStats.hpp"



#include <memory>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <array>


namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::graphics
{
    class Material;
    class Mesh;
    class ComputePass;
    class ComputeProgram;
    class Texture;
} //namespace vela::graphics

namespace vela::backend
{
    class RenderGraphImpl
    {
    public:
        RenderGraphImpl(core::Context& context);

        Status beginFrame();
        void endFrame();

        Status execute();

        void draw(const graphics::Mesh& mesh, const graphics::Material& material, const void* constants, uint32_t size);
        void bind(const graphics::Material& material);
        void bindVertexBuffer(const graphics::DynamicBuffer& buffer);
        void bindAttachment(uint32_t slot, const std::string& attachmentName);
        void bindTexture(uint32_t slot, const graphics::Texture& texture);
        void setConstantBytes(const void* data, uint32_t size);
        void bindIndexBuffer(const graphics::DynamicBuffer& buffer);
        void setConstants(const math::Mat4& value);
        void bindMaterialStorageBuffer(uint32_t slot, const std::string& bufferName);
        void drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
        void drawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset,
                 uint32_t instanceCount = 1, uint32_t firstInstance = 0);
        void setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

        void setPresentSource(std::string attachmentName);

        Status addPass(const std::string& name, std::unique_ptr<graphics::Pass> pass);
        Status addComputePass(const std::string& name, std::unique_ptr<graphics::ComputePass> pass);

        void bindComputeProgram(const graphics::ComputeProgram& program);
        void bindStorageBuffer(const graphics::ComputeProgram& program, uint32_t slot, const std::string& bufferName);
        void setComputeConstants(const math::Mat4& value);
        void dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ);

        void setView(const math::Mat4& view, const math::Mat4& projection);
        
        graphics::FrameStats getFrameStats() const;

        static constexpr uint32_t getFramesInFlight()
        {
            return k_framesInFlight;
        }

        ~RenderGraphImpl();
    private:
        bool m_isRenderGraphDirty{true};

        std::string m_presentAttachmentName;

        struct RegisteredPass
        {
            std::unique_ptr<Pass> pass;
            PassDeclaration declaration;
            PassFormats formats;
        };


        void runPass(RegisteredPass& registered);

        // Execution order is the vector order; the map is lookup by name only.
        std::vector<RegisteredPass> m_renderGraphPasses;
        std::unordered_map<std::string, size_t> m_renderGraphPassIndices;
        struct AttachmentState
        {
            VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
            VkPipelineStageFlags2 stage{VK_PIPELINE_STAGE_2_NONE};
            VkAccessFlags2 access{VK_ACCESS_2_NONE};
        };

        std::unordered_map<std::string, AttachmentState> m_attachmentStates;

        void transitionAttachment(const std::string& name, VkImageLayout newLayout, VkPipelineStageFlags2 dstStage,
            VkAccessFlags2 dstAccess, std::vector<VkImageMemoryBarrier2>& barriers);
        std::vector<size_t> m_executionOrder;
        std::unordered_map<std::string, std::vector<size_t>> m_attachmentProducers;

        RegisteredPass* m_currentRenderGraphPass{nullptr};

        MaterialImpl* m_boundMaterial{nullptr};
        VkPipeline m_boundPipeline{VK_NULL_HANDLE};
        VkPipelineLayout m_boundPipelineLayout{VK_NULL_HANDLE};
        uint32_t m_boundPushConstantSize{0};
        VkShaderStageFlags m_boundPushConstantStages{0};

        void bindMaterial(const graphics::Material& material);
        void pushConstants(const void* data, uint32_t size);
        VkBuffer m_boundVertexBuffer{VK_NULL_HANDLE};
        VkBuffer m_boundIndexBuffer{VK_NULL_HANDLE};
        bool m_currentPassOpenedRendering{false};

        static constexpr uint32_t k_framesInFlight = FrameConstants::k_framesInFlight;

        Pass& addRenderGraphPass(const std::string& name, std::unique_ptr<Pass> pass);

        void buildRenderGraphOrder();
        void cullRenderGraphOrder(const std::vector<std::vector<size_t>>& successors);

        void allocateAllRenderGraphPassOutputs();
        void allocateRenderGraphPassOutputs(const PassDeclaration& declaration,
            const std::unordered_map<std::string, VkImageUsageFlags>& usages);
        Attachment allocateAttachment(const AttachmentOutput& attachmentOutput, VkImageUsageFlags usage, bool isDepth);
        VkExtent2D passExtent(const PassDeclaration& declaration) const;

        void createSwapchainSyncObjects();
        void destroySwapchainSyncObjects();
        void recreateSwapchainResources();
        // Fixed-size attachments don't depend on the swapchain, so a resize keeps them
        void destroyAllAttachments(bool keepFixedSize = false);
        void resetAllAttachmentLayouts();

        core::Context& m_context;
 
        // Per frame-in-flight: waited on by the submit that renders into the
        // image this frame acquired
        std::vector<VkSemaphore> m_imageAvailable;
        std::vector<VkFence> m_inFlightFences;
        std::vector<VkCommandBuffer> m_commandBuffers;

        math::Mat4 m_view;
        math::Mat4 m_projection;

        std::vector<VkSemaphore> m_renderFinished;

        VkDevice m_device{VK_NULL_HANDLE};
        VkQueue m_graphicsQueue{VK_NULL_HANDLE};
        PipelineCache m_pipelineCache;

        bool m_isFrameValid{true};

        std::unordered_map<std::string, Attachment> m_attachments;
        std::unordered_map<std::string, GraphBuffer> m_buffers;

        void allocateGraphBuffers();
        void destroyAllGraphBuffers();
        void transitionGraphBuffers(const PassDeclaration& declaration, PassKind kind);

        const graphics::ComputeProgram* m_boundComputeProgram{nullptr};

        uint32_t m_frameIndex{0};
        uint32_t m_currentImageIndex{0};
        VkCommandBuffer m_currentCommandBuffer{VK_NULL_HANDLE};

        VkDescriptorPool m_descriptorPool{VK_NULL_HANDLE};
        std::array<VkDescriptorSet, k_framesInFlight> m_perViewDescriptorSets;

        /*
            Pass-input slots (set 2) are written into a fresh transient set whenever they change between draws,
            so a set is never updated after the command buffer has bound it. Transient pools are reset once
            the frame that used them has retired
        */
        struct InputBinding
        {
            VkImageView view{VK_NULL_HANDLE};
            VkSampler sampler{VK_NULL_HANDLE};
        };

        std::array<InputBinding, k_passInputSlots> m_inputBindings{};
        VkDescriptorSet m_currentInputSet{VK_NULL_HANDLE};
        bool m_inputBindingsDirty{false};

        // Sets 0 and 1 are bound lazily, right before a draw, so storage buffers bound after bind() still land
        bool m_materialSetsDirty{false};

        std::array<std::vector<VkDescriptorPool>, k_framesInFlight> m_transientPools;
        std::array<size_t, k_framesInFlight> m_transientPoolCursor{};

        VkDescriptorPool createTransientPool();
        VkDescriptorSet allocateTransientSet(VkDescriptorSetLayout layout);
        void flushDescriptors();
        std::array<VkBuffer, k_framesInFlight> m_perViewBuffer{VK_NULL_HANDLE};
        std::array<VmaAllocation, k_framesInFlight> m_perViewBufferAllocation{VK_NULL_HANDLE};
        std::array<void*, k_framesInFlight> m_perViewMapped{nullptr};

        VkQueryPool m_queryPool{VK_NULL_HANDLE};

        graphics::FrameStats m_frameStats;

        float m_timestampPeriod{0.0f};
        bool m_gpuTimingSupported{false};
        std::array<bool, k_framesInFlight> m_timestampsWritten{};
    };
} //namespace vela::backend


#endif //VELA_BACKEND_RENDER_GRAPH_IMPL_HPP
