#include "Vela/Graphics/RenderGraph.hpp"
#include "RenderGraphImpl.hpp"
#include "ContextImpl.hpp"
#include "Vela/Core/Context.hpp"

namespace vela::graphics
{
    RenderGraph::RenderGraph(core::Context& context) : m_impl(std::make_unique<backend::RenderGraphImpl>(context))
    {

    }

    RenderGraph::RenderGraph(RenderGraph&&) noexcept = default;

    RenderGraph& RenderGraph::operator=(RenderGraph&&) noexcept = default;

    void RenderGraph::beginFrame()
    {
        m_impl->beginFrame();
    }

    void RenderGraph::beginPresentPass(float r, float g, float b, float a)
    {
        m_impl->beginPresentPass(r, g, b, a);
    }

    void RenderGraph::endRenderPass()
    {
        m_impl->endRenderPass();
    }

    void RenderGraph::endFrame()
    {
        m_impl->endFrame();
    }

    void RenderGraph::draw(const graphics::Mesh& mesh, const graphics::Material& material, const glm::mat4& model)
    {
        m_impl->draw(mesh, material, model);
    }

    RenderGraph::~RenderGraph() = default;
} // namespace vela::graphics