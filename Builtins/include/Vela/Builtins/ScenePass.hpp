#ifndef VELA_BUILTINS_SCENE_PASS_HPP
#define VELA_BUILTINS_SCENE_PASS_HPP 

#include "Vela/Graphics/Pass.hpp"
#include "Vela/Graphics/RenderScene.hpp"

#include <cstdint>

namespace vela::builtins
{
    // Attachments the scene pass writes. Passes added after it read or load these names.
    // Color is linear HDR; depth is cleared to 1 and tested with Less.
    inline constexpr const char* k_sceneColorAttachment{"color"};
    inline constexpr const char* k_sceneDepthAttachment{"depth"};
    inline constexpr graphics::TextureFormat k_sceneColorFormat{graphics::TextureFormat::RGBA16Float};
    inline constexpr graphics::TextureFormat k_sceneDepthFormat{graphics::TextureFormat::Depth32Float};

    // The multisampled attachments the scene pass draws into when ScenePassOptions::samples > 1. They are
    // resolved into k_sceneColorAttachment and k_sceneDepthAttachment at the end of the pass, so passes
    // after it read the same names with or without MSAA. Load these instead to keep drawing multisampled.
    inline constexpr const char* k_sceneColorMsaaAttachment{"colorMsaa"};
    inline constexpr const char* k_sceneDepthMsaaAttachment{"depthMsaa"};

    // How the scene pass draws.
    struct ScenePassOptions
    {
        // Color the scene is cleared to before drawing (linear). Only r, g, b and a are used.
        graphics::ClearValue clear{0.1f, 0.2f, 0.4f, 1.0f};

        // Samples per pixel. 1 disables MSAA; otherwise a power of two up to core::DeviceInfo::maxMsaaSamples,
        // or addPass()/forwardGraph() fails.
        uint32_t samples{1};
    };

    // Draws every submission of a RenderScene with its material into k_sceneColorAttachment and
    // k_sceneDepthAttachment.
    class ScenePass : public graphics::Pass
    {
    public:
        // renderScene: the scene to draw each frame; must outlive the pass.
        explicit ScenePass(const graphics::RenderScene& renderScene);

        // renderScene: the scene to draw each frame; must outlive the pass.
        // options:     clear color and MSAA sample count.
        ScenePass(const graphics::RenderScene& renderScene, const ScenePassOptions& options);

        graphics::PassDescription describe() const override;
        void record(graphics::PassRecorder& recorder) override;
    private:
        const graphics::RenderScene* m_renderScene{nullptr};
        ScenePassOptions m_options{};
    };
} //namespace vela::builtins

#endif //VELA_BUILTINS_SCENE_PASS_HPP
