#ifndef VELA_GRAPHICS_RENDER_GRAPH_HPP
#define VELA_GRAPHICS_RENDER_GRAPH_HPP

#include <memory>

#include "Vela/Result.hpp"
#include "Vela/Math/Matrix.hpp"

namespace vela::core
{
    class Context;
} //namespace vela::core

namespace vela::backend
{
    class RenderGraphImpl;
} //namespace vela::backend

namespace vela::graphics
{
    class Mesh;
    class Material;
} // namespace vela::graphics

namespace vela::graphics
{
    class RenderGraph
    {
    public:
        static Result<RenderGraph> create(core::Context& context);

        RenderGraph(RenderGraph&&) noexcept;
        RenderGraph& operator=(RenderGraph&&) noexcept;

        RenderGraph(const RenderGraph&) = delete;
        RenderGraph& operator=(const RenderGraph&) = delete;

        void beginFrame();

        void beginPass(const std::string& renderGraphPassName);
        void draw(const graphics::Mesh& mesh, const graphics::Material& material, const math::Mat4& model);
        void updatePerViewDescriptors(const math::Mat4& view, const math::Mat4& projection);
        void endPass();
        void endFrame();

        backend::RenderGraphImpl* impl() const;

    private:
        RenderGraph(core::Context& context);

    public:

        ~RenderGraph();

    private:

        std::unique_ptr<backend::RenderGraphImpl> m_impl{nullptr};
    };

} //namespace vela::graphics

#endif //VELA_GRAPHICS_RENDER_GRAPH_HPP