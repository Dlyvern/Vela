#ifndef VELA_GRAPHICS_RENDER_GRAPH_HPP
#define VELA_GRAPHICS_RENDER_GRAPH_HPP

#include <memory>

#include "Vela/Result.hpp"
#include "Vela/Math/Matrix.hpp"
#include "FrameStats.hpp"

#include "Pass.hpp"

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

        [[nodiscard]] Status execute();

        void setView(const math::Mat4& view, const math::Mat4& projection);

        [[nodiscard]] Status addPass(const std::string& name, std::unique_ptr<Pass> pass);

        void setPresentSource(const std::string& attachmentName);

        backend::RenderGraphImpl* impl() const;

        [[nodiscard]]FrameStats getFrameStats() const;

        uint32_t getFramesInFlight();

    private:
        RenderGraph(core::Context& context);

    public:

        ~RenderGraph();

    private:

        std::unique_ptr<backend::RenderGraphImpl> m_impl{nullptr};
    };

} //namespace vela::graphics

#endif //VELA_GRAPHICS_RENDER_GRAPH_HPP