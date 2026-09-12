#include "Vela/Builtins/ScenePass.hpp"

namespace vela::builtins
{
    ScenePass::ScenePass(const graphics::RenderScene& renderScene) : m_renderScene(&renderScene)
    {
        
    }

    graphics::PassDescription ScenePass::describe() const
    {
        graphics::AttachmentSlot colorSlot{};
        colorSlot.format = graphics::TextureFormat::RGBA16Float;
        colorSlot.load = graphics::LoadOp::Clear;
        colorSlot.store = graphics::StoreOp::Store;
        colorSlot.name = "color";
        colorSlot.clear = m_clearValue;

        graphics::AttachmentSlot depthSlot{};
        depthSlot.format = graphics::TextureFormat::Depth32Float;
        depthSlot.load = graphics::LoadOp::Clear;
        depthSlot.store = graphics::StoreOp::Store;
        depthSlot.name = "depth";

        graphics::PassDescription passDescription{};
        passDescription.colorOutputs.push_back(colorSlot);
        passDescription.depthOutput = depthSlot;

        return passDescription;
    }

    void ScenePass::record(graphics::PassRecorder& recorder)
    {
        for(const auto& submission : m_renderScene->submissions())
            recorder.draw(*submission.mesh, *submission.material, submission.model);
    }
} //namespace vela::builtins