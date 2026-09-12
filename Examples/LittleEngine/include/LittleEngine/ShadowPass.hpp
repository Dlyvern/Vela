#ifndef LITTLE_ENGINE_SHADOW_PASS_HPP
#define LITTLE_ENGINE_SHADOW_PASS_HPP

#include "Vela/Graphics/Pass.hpp"
#include "Vela/Graphics/RenderScene.hpp"

namespace little
{
    class ShadowPass : public vela::graphics::Pass
    {
    public:
        ShadowPass(vela::core::Context& context, vela::graphics::RenderScene* renderScene);
        vela::graphics::PassDescription describe() const override;
        void record(vela::graphics::PassRecorder& recorder) override;

        void setLightViewProjection(const vela::math::Mat4& lightViewProjection);

    private:
        std::optional<vela::graphics::Material> m_material;
        vela::graphics::RenderScene* m_renderScene{nullptr};
        vela::math::Mat4 m_lightViewProjection{1.0f};
    };
} //namespace little

#endif //LITTLE_ENGINE_SHADOW_PASS_HPP