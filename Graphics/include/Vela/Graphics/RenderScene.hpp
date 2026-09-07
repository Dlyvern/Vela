#ifndef VELA_GRAPHICS_RENDER_SCENE_HPP
#define VELA_GRAPHICS_RENDER_SCENE_HPP

#include "Vela/Math/Matrix.hpp"

#include <span>
#include <vector>

namespace vela::graphics
{
    class Mesh;
    class Material;

    struct Submission
    {
        const Mesh* mesh{nullptr};
        const Material* material{nullptr};
        math::Mat4 model{1.0f};
    };

    class RenderScene
    {
    public:
        void add(const Mesh& mesh, const Material& material, const math::Mat4& model);

        void clear();

        [[nodiscard]] std::span<const Submission> submissions() const;

    private:
        std::vector<Submission> m_submissions;
    };

} //namespace vela::graphics

#endif //VELA_GRAPHICS_RENDER_SCENE_HPP