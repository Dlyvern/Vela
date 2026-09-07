#include "Vela/Graphics/RenderGraph.hpp"

#include "RenderGraphImpl.hpp"
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

    Status RenderGraph::addPass(const std::string& name, std::unique_ptr<Pass> pass)
    {
        return m_impl->addPass(name, std::move(pass));
    }

    FrameStats RenderGraph::getFrameStats() const
    {
        return m_impl->getFrameStats();
    }

    void RenderGraph::setPresentSource(const std::string& attachmentName)
    {
        m_impl->setPresentSource(attachmentName);
    }

    Status RenderGraph::execute()
    {
        return m_impl->execute();
    }

    void RenderGraph::setView(const math::Mat4& view, const math::Mat4& projection)
    {
        m_impl->setView(view, projection);
    }

    backend::RenderGraphImpl* RenderGraph::impl() const
    {
        return m_impl.get();
    }

    RenderGraph::~RenderGraph() = default;
} // namespace vela::graphics