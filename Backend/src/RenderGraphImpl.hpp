#ifndef VELA_BACKEND_RENDER_GRAPH_IMPL_HPP
#define VELA_BACKEND_RENDER_GRAPH_IMPL_HPP

#include "volk.h"

#include "PresentPass.hpp"

#include <memory>

#include <cstdint>
#include <vector>
#include <glm/mat4x4.hpp>

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
        VkFormat getDepthFormat() const;

        void draw(const graphics::Mesh& mesh, const graphics::Material& material, const glm::mat4& model);

        void endRenderPass();

        ~RenderGraphImpl();

    private:
        // How many frames the CPU may run ahead of the GPU. One would mean the
        // CPU blocks on the previous frame before it can record the next one,
        // so every GPU/present stall lands directly in the frame time
        static constexpr uint32_t k_framesInFlight{2};

        PassContext makePassContext() const;

        void createSwapchainSyncObjects();
        void destroySwapchainSyncObjects();
        void recreateSwapchainResources();

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

        bool m_isFrameValid{true};

        uint32_t m_frameIndex{0};
        uint32_t m_currentImageIndex{0};
        VkCommandBuffer m_currentCommandBuffer{VK_NULL_HANDLE};
    };
} //namespace vela::backend


#endif //VELA_BACKEND_RENDER_GRAPH_IMPL_HPP
