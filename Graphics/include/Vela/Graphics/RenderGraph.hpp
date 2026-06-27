#ifndef VELA_GRAPHICS_RENDER_GRAPH_HPP
#define VELA_GRAPHICS_RENDER_GRAPH_HPP

#include <memory>
#include "glm/mat4x4.hpp"

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::backend
{
    class RenderGraphImpl;
} //namespace vela::backend

namespace vela::graphics
{
    class Mesh;
    class Material;
} // namespace vela::graphics


namespace vela::graphics
{
    class RenderGraph
    {
    public:
        RenderGraph(core::Context& context);

        RenderGraph(RenderGraph&&) noexcept;
        RenderGraph& operator=(RenderGraph&&) noexcept;

        RenderGraph(const RenderGraph&) = delete;
        RenderGraph& operator=(const RenderGraph&) = delete;


        void beginFrame();
        void beginPresentPass(float r = 0.1f, float g = 0.2f, float b = 0.4f, float a = 1.0f);
        void draw(const graphics::Mesh& mesh, const graphics::Material& material, const glm::mat4& model);
        void endRenderPass();
        void endFrame();

        ~RenderGraph();

    private:
        std::unique_ptr<backend::RenderGraphImpl> m_impl{nullptr};
    };

} //namespace vela::graphics

#endif //VELA_GRAPHICS_RENDER_GRAPH_HPP