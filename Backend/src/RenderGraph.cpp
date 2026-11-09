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

    Status RenderGraph::addComputePass(const std::string& name, std::unique_ptr<ComputePass> pass)
    {
        return m_impl->addComputePass(name, std::move(pass));
    }

    Status RenderGraph::removePass(const std::string& name)
    {
        return m_impl->removePass(name);
    }

    Status RenderGraph::redescribePass(const std::string& name)
    {
        return m_impl->redescribePass(name);
    }

    FrameStats RenderGraph::getFrameStats() const
    {
        return m_impl->getFrameStats();
    }

    const std::vector<PassTiming>& RenderGraph::getPassTimings() const
    {
        return m_impl->getPassTimings();
    }

    void RenderGraph::setPresentSource(const std::string& attachmentName)
    {
        m_impl->setPresentSource(attachmentName);
    }

    Status RenderGraph::readbackFrame(std::function<void(const ImageData&)> onReady)
    {
        return m_impl->readbackFrame(std::move(onReady));
    }

    Status RenderGraph::execute()
    {
        // Pipeline creation, graph compilation and attachment allocation report failure by throwing internally
        try
        {
            return m_impl->execute();
        }
        catch (const std::exception& error)
        {
            return Error{ErrorCode::Unknown, error.what()};
        }
    }

    void RenderGraph::setView(const math::Mat4& view, const math::Mat4& projection)
    {
        m_impl->setView(view, projection);
    }

    uint32_t RenderGraph::getFramesInFlight()
    {
        return m_impl->getFramesInFlight();
    }

    backend::RenderGraphImpl* RenderGraph::impl() const
    {
        return m_impl.get();
    }

    RenderGraph::~RenderGraph() = default;
} // namespace vela::graphics