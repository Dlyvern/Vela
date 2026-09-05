#include "Vela/Graphics/RenderGraph.hpp"

#include <stdexcept>
#include "RenderGraphImpl.hpp"
#include "ContextImpl.hpp"
#include "Vela/Core/Context.hpp"

namespace vela::graphics
{
    Result<RenderGraph> RenderGraph::create(core::Context& context)
    {
        try
        {
            return RenderGraph(context);
        }
        catch (const std::exception& error)
        {
            return Error{ErrorCode::DeviceCreationFailed, error.what()};
        }
    }

    RenderGraph::RenderGraph(core::Context& context) : m_impl(std::make_unique<backend::RenderGraphImpl>(context))
    {

    }

    RenderGraph::RenderGraph(RenderGraph&&) noexcept = default;

    RenderGraph& RenderGraph::operator=(RenderGraph&&) noexcept = default;

    void RenderGraph::beginFrame()
    {
        m_impl->beginFrame();
    }

    void RenderGraph::updatePerViewDescriptors(const math::Mat4& view, const math::Mat4& projection)
    {
        m_impl->updatePerViewDescriptors(view, projection);
    }

    void RenderGraph::beginPass(const std::string& renderGraphPassName)
    {
        m_impl->beginPass(renderGraphPassName);
    }

    void RenderGraph::endPass()
    {
        m_impl->endPass();
    }

    void RenderGraph::endFrame()
    {
        m_impl->endFrame();
    }

    void RenderGraph::draw(const graphics::Mesh& mesh, const graphics::Material& material, const math::Mat4& model)
    {
        m_impl->draw(mesh, material, model);
    }

    backend::RenderGraphImpl* RenderGraph::impl() const
    {
        return m_impl.get();
    }

    RenderGraph::~RenderGraph() = default;
} // namespace vela::graphics