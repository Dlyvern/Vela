#ifndef LITTLE_ENGINE_LIT_PASS_HPP
#define LITTLE_ENGINE_LIT_PASS_HPP

#include "Vela/Graphics/Pass.hpp"
#include "Vela/Graphics/RenderScene.hpp"

namespace little 
{
    class LitPass : public vela::graphics::Pass
    {
    public:
        explicit LitPass(const vela::graphics::RenderScene& renderScene);

        vela::graphics::PassDescription describe() const override;

        void record(vela::graphics::PassRecorder& recorder) override;
    private:
        const vela::graphics::RenderScene* m_renderScene{nullptr};
        vela::graphics::ClearValue m_clearValue{0.1f, 0.2f, 0.4f, 1.0f};
    };
} //namespace little 

#endif //LITTLE_ENGINE_LIT_PASS_HPP