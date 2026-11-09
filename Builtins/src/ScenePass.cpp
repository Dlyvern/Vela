#include "Vela/Builtins/ScenePass.hpp"

namespace vela::builtins
{
    ScenePass::ScenePass(const graphics::RenderScene& renderScene) : m_renderScene(&renderScene)
    {
        
    }

    ScenePass::ScenePass(const graphics::RenderScene& renderScene, const ScenePassOptions& options) :
        m_renderScene(&renderScene), m_options(options)
    {

    }

    graphics::PassDescription ScenePass::describe() const
    {
        const bool multisampled = m_options.samples > 1;

        graphics::AttachmentSlot colorSlot{};
        colorSlot.format = k_sceneColorFormat;
        colorSlot.load = graphics::LoadOp::Clear;
        colorSlot.store = graphics::StoreOp::Store;
        colorSlot.name = multisampled ? k_sceneColorMsaaAttachment : k_sceneColorAttachment;
        colorSlot.clear = m_options.clear;
        colorSlot.samples = m_options.samples;

        graphics::AttachmentSlot depthSlot{};
        depthSlot.format = k_sceneDepthFormat;
        depthSlot.load = graphics::LoadOp::Clear;
        depthSlot.store = graphics::StoreOp::Store;
        depthSlot.name = multisampled ? k_sceneDepthMsaaAttachment : k_sceneDepthAttachment;
        depthSlot.samples = m_options.samples;

        if (multisampled)
        {
            colorSlot.resolveTarget = k_sceneColorAttachment;
            depthSlot.resolveTarget = k_sceneDepthAttachment;
        }

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
