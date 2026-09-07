#include "Vela/Graphics/RenderScene.hpp"

#include "Vela/Graphics/Material.hpp"
#include "Vela/Graphics/Mesh.hpp"


namespace vela::graphics
{
    void RenderScene::add(const Mesh& mesh, const Material& material, const math::Mat4& model)
    {
        m_submissions.push_back({.mesh = &mesh, .material = &material, .model = model});
    }

    void RenderScene::clear()
    {
        m_submissions.clear();
    }

    [[nodiscard]] std::span<const Submission> RenderScene::submissions() const
    {
        return m_submissions;
    }
} //namespace vela::graphics