#ifndef VELA_BUILTINS_SCENE_PASS_HPP
#define VELA_BUILTINS_SCENE_PASS_HPP 

#include "Vela/Graphics/Pass.hpp"
#include "Vela/Graphics/RenderScene.hpp"

namespace vela::builtins
{   
    class ScenePass : public graphics::Pass
    {
    public:
        explicit ScenePass(const graphics::RenderScene& renderScene);
        graphics::PassDescription describe() const override;
        void record(graphics::PassRecorder& recorder) override;
    private:
        const graphics::RenderScene* m_renderScene{nullptr};
        graphics::ClearValue m_clearValue{0.1f, 0.2f, 0.4f, 1.0f};
    };
} //namespace vela::builtins

#endif //VELA_BUILTINS_SCENE_PASS_HPP