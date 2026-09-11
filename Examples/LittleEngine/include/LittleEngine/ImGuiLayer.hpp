#ifndef LITTLE_ENGINE_IMGUI_LAYER_HPP
#define LITTLE_ENGINE_IMGUI_LAYER_HPP

#include "Vela/Core/Context.hpp"
#include "Vela/Core/Input.hpp"
#include "Vela/Core/Window.hpp"
#include "Vela/Graphics/Material.hpp"
#include "Vela/Graphics/Pass.hpp"
#include "Vela/Graphics/Texture.hpp"
#include "Vela/Result.hpp"

#include "imgui.h"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace little
{
    class ImGuiPass : public vela::graphics::Pass
    {
    public:
        ImGuiPass(vela::core::Context& context, vela::graphics::Material& material,
            const std::string& colorAttachment);

        vela::graphics::PassDescription describe() const override;
        void record(vela::graphics::PassRecorder& recorder) override;

    private:
        vela::graphics::Material& m_material;
        std::string m_colorAttachment;

        std::vector<ImDrawVert> m_vertices;
        std::vector<ImDrawIdx> m_indices;

        vela::graphics::DynamicBuffer m_vertexBuffer;
        vela::graphics::DynamicBuffer m_indexBuffer;
    };

    class ImGuiLayer
    {
    public:
        [[nodiscard]] vela::Status initialise(vela::core::Context& context,
            std::span<const uint32_t> vertexShader, std::span<const uint32_t> fragmentShader);

        void handleEvent(const vela::core::Event& event);
        void beginFrame(vela::core::Window& window, float deltaTime);

        [[nodiscard]] std::unique_ptr<ImGuiPass> createPass(const std::string& colorAttachment);

        [[nodiscard]] bool wantsMouse() const;
        [[nodiscard]] bool wantsKeyboard() const;

        ~ImGuiLayer();

    private:
        vela::core::Context* m_context{nullptr};
        std::unique_ptr<vela::graphics::Material> m_material;
        std::unique_ptr<vela::graphics::Texture> m_fontTexture;

        bool m_initialised{false};
    };
} //namespace little

#endif //LITTLE_ENGINE_IMGUI_LAYER_HPP
