#include "LittleEngine/LitPass.hpp"

namespace little
{
    LitPass::LitPass(const vela::graphics::RenderScene& renderScene) : m_renderScene(&renderScene)
    {

    }
    
    vela::graphics::PassDescription LitPass::describe() const
    {   
        vela::graphics::AttachmentSlot colorSlot{};
        colorSlot.format = vela::graphics::TextureFormat::RGBA16Float;
        colorSlot.load = vela::graphics::LoadOp::Clear;
        colorSlot.store = vela::graphics::StoreOp::Store;
        colorSlot.name = "color";
        colorSlot.clear = m_clearValue;

        vela::graphics::AttachmentSlot depthSlot{};
        depthSlot.format = vela::graphics::TextureFormat::Depth32Float;
        depthSlot.load = vela::graphics::LoadOp::Clear;
        depthSlot.store = vela::graphics::StoreOp::Store;
        depthSlot.name = "depth";

        vela::graphics::PassInput shadowInput{};
        shadowInput.name = "shadowMap";
        shadowInput.usage = vela::graphics::InputUsage::Sampled;

        vela::graphics::PassDescription passDescription{};
        passDescription.colorOutputs.push_back(colorSlot);
        passDescription.depthOutput = depthSlot;
        passDescription.inputs.push_back(shadowInput);

        return passDescription;
    }

    void LitPass::record(vela::graphics::PassRecorder& recorder)
    {
        recorder.bindAttachment(0, "shadowMap");

        for(const auto& submission : m_renderScene->submissions())
            recorder.draw(*submission.mesh, *submission.material, submission.model);
    }
} //namespace little