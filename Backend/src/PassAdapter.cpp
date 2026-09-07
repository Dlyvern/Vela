#include "PassAdapter.hpp"

#include "Formats.hpp"
#include "RenderGraphImpl.hpp"

namespace
{
    vela::backend::AttachmentOutput toAttachmentOutput(const vela::graphics::AttachmentSlot& slot, bool isDepth)
    {
        vela::backend::AttachmentOutput output;
        output.name = slot.name;
        output.format = vela::backend::toVkFormat(slot.format);
        output.load = vela::backend::toVkLoadOp(slot.load);
        output.store = vela::backend::toVkStoreOp(slot.store);
        output.clear = vela::backend::toVkClearValue(slot.clear, isDepth);
        output.scale = slot.scale;

        return output;
    }
} //namespace

namespace vela::graphics
{
    void PassRecorder::draw(const Mesh& mesh, const Material& material, const math::Mat4& model)
    {
        if (m_graph == nullptr)
            return;

        m_graph->draw(mesh, material, model);
    }

    Extent2D PassRecorder::extent() const
    {
        return m_extent;
    }
} //namespace vela::graphics

namespace vela::backend
{
    PassAdapter::PassAdapter(std::unique_ptr<graphics::Pass> pass, RenderGraphImpl& renderGraph)
        : m_pass(std::move(pass)), m_renderGraph(renderGraph)
    {
        if (m_pass == nullptr)
            throw std::runtime_error("PassAdapter requires a pass");
    }

    PassDeclaration PassAdapter::declare() const
    {
        const graphics::PassDescription description = m_pass->describe();

        PassDeclaration declaration;

        declaration.colorOutputs.reserve(description.colorOutputs.size());

        for (const graphics::AttachmentSlot& slot : description.colorOutputs)
            declaration.colorOutputs.push_back(toAttachmentOutput(slot, false));

        if (description.depthOutput.has_value())
            declaration.depthOutput = toAttachmentOutput(description.depthOutput.value(), true);

        declaration.inputs.reserve(description.inputs.size());

        for (const graphics::PassInput& input : description.inputs)
            declaration.inputs.push_back(PassInput{input.name, toBackendInputUsage(input.usage)});

        return declaration;
    }

    void PassAdapter::record(const PassContext& passContext)
    {
        graphics::PassRecorder recorder;
        recorder.m_graph = &m_renderGraph;
        recorder.m_extent = graphics::Extent2D{passContext.extent.width, passContext.extent.height};

        m_pass->record(recorder);
    }
} //namespace vela::backend
