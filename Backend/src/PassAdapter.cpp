#include "PassAdapter.hpp"

#include "Formats.hpp"
#include "RenderGraphImpl.hpp"

#include <stdexcept>

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
        output.size.height = slot.size.height;
        output.size.width = slot.size.width;

        return output;
    }
} //namespace

namespace vela::graphics
{
    void PassRecorder::draw(const Mesh& mesh, const Material& material, const math::Mat4& model)
    {
        if (!m_graph)
            return;

        m_graph->draw(mesh, material, model);
    }

    void PassRecorder::bind(const Material& material)
    {
        if (!m_graph)
            return;

        m_graph->bind(material);
    }

    void PassRecorder::bindVertexBuffer(const DynamicBuffer& buffer)
    {
        if (!m_graph)
            return;

        m_graph->bindVertexBuffer(buffer);
    }

    void PassRecorder::bindIndexBuffer(const DynamicBuffer& buffer)
    {
        if (!m_graph)
            return;

        m_graph->bindIndexBuffer(buffer);
    }

    void PassRecorder::drawInstanced(uint32_t vertexCount, uint32_t instanceCount,
        uint32_t firstVertex, uint32_t firstInstance)
    {
        if (!m_graph)
            return;

        m_graph->drawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void PassRecorder::bindStorageBuffer(uint32_t slot, const std::string& bufferName)
    {
        if (!m_graph)
            return;

        m_graph->bindMaterialStorageBuffer(slot, bufferName);
    }

    void PassRecorder::bindAttachment(uint32_t slot, const std::string& attachmentName)
    {
        if(!m_graph)
            return;

        m_graph->bindAttachment(slot, attachmentName);
    }

    void PassRecorder::setConstants(const math::Mat4& value)
    {
        if (!m_graph)
            return;

        m_graph->setConstants(value);
    }

    void PassRecorder::setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
    {
        m_graph->setScissor(x, y, width, height);
    }

    void PassRecorder::drawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset,
                 uint32_t instanceCount, uint32_t firstInstance)
    {
        if (!m_graph)
            return;

        m_graph->drawIndexed(indexCount, firstIndex, vertexOffset, instanceCount, firstInstance);
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

        declaration.bufferOutputs.reserve(description.bufferOutputs.size());

        for (const graphics::BufferSlot& slot : description.bufferOutputs)
            declaration.bufferOutputs.push_back(BufferOutput{slot.name, slot.size});

        declaration.bufferInputs.reserve(description.bufferInputs.size());

        for (const graphics::BufferInput& input : description.bufferInputs)
            declaration.bufferInputs.push_back(BufferInput{input.name,
                input.access == graphics::BufferAccess::ReadWrite ? BufferAccess::ReadWrite : BufferAccess::Read});

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
