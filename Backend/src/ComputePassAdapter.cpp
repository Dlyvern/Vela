#include "ComputePassAdapter.hpp"

#include "RenderGraphImpl.hpp"

namespace vela::graphics
{
    void ComputeRecorder::bind(const ComputeProgram& program)
    {
        if (!m_graph)
            return;

        m_program = &program;
        m_graph->bindComputeProgram(program);
    }

    void ComputeRecorder::bindStorageBuffer(uint32_t slot, const std::string& bufferName)
    {
        if (!m_graph || !m_program)
            return;

        m_graph->bindStorageBuffer(*m_program, slot, bufferName);
    }

    void ComputeRecorder::setConstants(const math::Mat4& value)
    {
        if (!m_graph)
            return;

        m_graph->setComputeConstants(value);
    }

    void ComputeRecorder::dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
    {
        if (!m_graph)
            return;

        m_graph->dispatch(groupsX, groupsY, groupsZ);
    }
} //namespace vela::graphics

namespace vela::backend
{
    ComputePassAdapter::ComputePassAdapter(std::unique_ptr<graphics::ComputePass> pass, RenderGraphImpl& renderGraph)
        : m_pass(std::move(pass)), m_renderGraph(renderGraph)
    {
    }

    PassKind ComputePassAdapter::kind() const
    {
        return PassKind::Compute;
    }

    PassDeclaration ComputePassAdapter::declare() const
    {
        const graphics::PassDescription description = m_pass->describe();

        PassDeclaration declaration;

        declaration.bufferOutputs.reserve(description.bufferOutputs.size());

        for (const graphics::BufferSlot& slot : description.bufferOutputs)
            declaration.bufferOutputs.push_back(BufferOutput{slot.name, slot.size});

        declaration.bufferInputs.reserve(description.bufferInputs.size());

        for (const graphics::BufferInput& input : description.bufferInputs)
            declaration.bufferInputs.push_back(BufferInput{input.name,
                input.access == graphics::BufferAccess::ReadWrite ? BufferAccess::ReadWrite : BufferAccess::Read});

        return declaration;
    }

    void ComputePassAdapter::record(const PassContext& passContext)
    {
        graphics::ComputeRecorder recorder;
        recorder.m_graph = &m_renderGraph;

        m_pass->record(recorder);
    }
} //namespace vela::backend
