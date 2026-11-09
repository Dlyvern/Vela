#include "LittleEngine/LitPass.hpp"

namespace little
{
    LitPass::LitPass(const vela::graphics::RenderScene& renderScene, uint32_t samples) :
        m_renderScene(&renderScene), m_samples(samples)
    {

    }
    
    vela::graphics::PassDescription LitPass::describe() const
    {   
        vela::graphics::AttachmentSlot colorSlot{};
        colorSlot.format = vela::graphics::TextureFormat::RGBA16Float;
        colorSlot.load = vela::graphics::LoadOp::Clear;
        colorSlot.store = vela::graphics::StoreOp::Store;
        colorSlot.name = m_samples > 1 ? "colorMsaa" : "color";
        colorSlot.clear = m_clearValue;
        colorSlot.samples = m_samples;

        vela::graphics::AttachmentSlot depthSlot{};
        depthSlot.format = vela::graphics::TextureFormat::Depth32Float;
        depthSlot.load = vela::graphics::LoadOp::Clear;
        depthSlot.store = vela::graphics::StoreOp::Store;
        depthSlot.name = m_samples > 1 ? "depthMsaa" : "depth";
        depthSlot.samples = m_samples;

        if (m_samples > 1)
        {
            colorSlot.resolveTarget = "color";
            depthSlot.resolveTarget = "depth";
        }

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
        // Hardware depth comparison: with linear filtering each lookup is a 2x2 PCF
        vela::graphics::SamplerDescription shadowSampler{};
        shadowSampler.addressU = vela::graphics::AddressMode::ClampToEdge;
        shadowSampler.addressV = vela::graphics::AddressMode::ClampToEdge;
        shadowSampler.addressW = vela::graphics::AddressMode::ClampToEdge;
        shadowSampler.mipMode = vela::graphics::MipMode::Nearest;
        shadowSampler.compareOp = vela::graphics::DepthCompare::LessOrEqual;

        recorder.bindAttachment(0, "shadowMap", shadowSampler);

        for(const auto& submission : m_renderScene->submissions())
            recorder.draw(*submission.mesh, *submission.material, submission.model);
    }
} //namespace little