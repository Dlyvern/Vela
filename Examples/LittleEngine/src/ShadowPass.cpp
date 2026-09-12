#include "LittleEngine/ShadowPass.hpp"

#include "Vela/Assets//Shader.hpp"
#include "Vela/Assets/Resources.hpp"

namespace little
{   
    ShadowPass::ShadowPass(vela::core::Context& context, vela::graphics::RenderScene* renderScene) :
    m_renderScene(renderScene)
    {
        vela::graphics::MaterialDescription materialDescription{};
        auto shadowVertResult = vela::assets::loadSpirv(vela::assets::resources::find("shaders/shadow.vert.spv").string());
        auto staticFragResult = vela::assets::loadSpirv(vela::assets::resources::find("shaders/shadow.frag.spv").string());

        if (!shadowVertResult) 
            throw std::runtime_error("Failed to create vertex shader");

        if (!staticFragResult)
            throw std::runtime_error("Failed to create fragment shader");

        const std::vector<uint32_t> shadowVert = std::move(shadowVertResult).value();
        const std::vector<uint32_t> staticFrag = std::move(staticFragResult).value();

        vela::graphics::RenderState renderState{};
        materialDescription.renderState = renderState;
        materialDescription.vertexLayout = vela::graphics::StaticVertex::layout();
        materialDescription.vertexShader = shadowVert;
        materialDescription.fragmentShader = staticFrag;

        auto shadowMaterial = vela::graphics::Material::create(context, materialDescription);

        if(!shadowMaterial)
            throw std::runtime_error("Failed to create shadow material");

        m_material = std::move(shadowMaterial).value();
    }

    vela::graphics::PassDescription ShadowPass::describe() const
    {
        vela::graphics::AttachmentSlot depthOutput{};
        depthOutput.format = vela::graphics::TextureFormat::Depth32Float;
        depthOutput.size = {2048, 2048};
        depthOutput.load = vela::graphics::LoadOp::Clear;
        depthOutput.store = vela::graphics::StoreOp::Store;
        depthOutput.name = "shadowMap";
        depthOutput.clear.depth = 1.0f;

        vela::graphics::PassDescription passDescription{};
        passDescription.depthOutput = depthOutput;

        return passDescription;
    }

    void ShadowPass::record(vela::graphics::PassRecorder& recorder)
    {
        if(!m_renderScene || !m_material.has_value())
            return;

        for(const auto& submission : m_renderScene->submissions())
            recorder.draw(*submission.mesh, m_material.value(), m_lightViewProjection * submission.model);
    }

    void ShadowPass::setLightViewProjection(const vela::math::Mat4& lightViewProjection)
    {
        m_lightViewProjection = lightViewProjection;
    }

} //namespace little