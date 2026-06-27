#ifndef VELA_BACKEND_RENDER_GRAPH_IMPL_HPP
#define VELA_BACKEND_RENDER_GRAPH_IMPL_HPP

#include "volk.h"

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

namespace vela::backend
{
    class RenderGraphImpl
    {
    public:
        RenderGraphImpl(core::Context& context);

        void beginFrame();
        void endFrame();

        void beginPresentPass(float r, float g, float b, float a);

        void draw(const graphics::Mesh& mesh, const graphics::Material& material, const glm::mat4& model);

        void endRenderPass();

        ~RenderGraphImpl();

    private:
        void recreateSwapchainResources();

        core::Context& m_context;

        VkSemaphore m_imageAvailable{VK_NULL_HANDLE};
        VkSemaphore m_renderFinished{VK_NULL_HANDLE};
        VkFence m_inFlightFence{VK_NULL_HANDLE};

        VkDevice m_device{VK_NULL_HANDLE};
        VkQueue m_graphicsQueue{VK_NULL_HANDLE};

        std::vector<VkCommandBuffer> m_commandBuffers;

        bool m_isFrameValid{true};

        uint32_t m_currentImageIndex{0};
        VkCommandBuffer m_currentCommandBuffer{VK_NULL_HANDLE};
    };
} //namespace vela::backend


#endif //VELA_BACKEND_RENDER_GRAPH_IMPL_HPP