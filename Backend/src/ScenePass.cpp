#include "ScenePass.hpp"

#include <iostream>
#include <array>

namespace vela::backend
{
    ScenePass::ScenePass()
    {
        
    }

    PassDeclaration ScenePass::declare() const
    {
        AttachmentOutput colorOutput{};
        colorOutput.clear = m_clearValue;
        colorOutput.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        colorOutput.name = "color";
        colorOutput.load = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorOutput.store = VK_ATTACHMENT_STORE_OP_STORE;

        AttachmentOutput depthOutput{};
        depthOutput.clear.depthStencil = {1.0f, 0};
        depthOutput.format = m_depthFormat;
        depthOutput.name = "depth";
        depthOutput.load = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthOutput.store = VK_ATTACHMENT_STORE_OP_STORE;

        PassDeclaration declaration{};
        declaration.colorOutputs.push_back(colorOutput);
        declaration.depthOutput = depthOutput;

        return declaration;
    }

    void ScenePass::record(const PassContext& passContext)
    {

    }
} //namespace vela::backend
