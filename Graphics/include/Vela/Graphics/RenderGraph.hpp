#ifndef VELA_GRAPHICS_RENDER_GRAPH_HPP
#define VELA_GRAPHICS_RENDER_GRAPH_HPP

#include <memory>
#include <string>
#include <vector>

#include "Vela/Result.hpp"
#include "Vela/Math/Matrix.hpp"
#include "FrameStats.hpp"
#include "ImageData.hpp"

#include <functional>

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

        /*
            Adds a pass. Its describe() is read now. Passes are ordered by what they declare: a pass runs after the
            producers of every attachment and buffer it lists as an input, and a pass that writes an attachment
            with LoadOp::Load runs after every pass added before it that writes the same attachment (so overlays
            added later draw on top). Passes nothing reaches from the present source are culled.
            name:    unique name of the pass.
            pass:    the pass; the graph owns it.
            Returns: an error if the name is taken, the pass is null, or its attachments are invalid
                     (e.g. an MSAA sample count the device can't render).
        */
        [[nodiscard]] Status addPass(const std::string& name, std::unique_ptr<Pass> pass);
        [[nodiscard]] Status addComputePass(const std::string& name, std::unique_ptr<ComputePass> pass);

        /*
            Removes a pass added with addPass() or addComputePass() and destroys it. Attachments and buffers are
            rebuilt on the next execute(); GPU resources still used by frames in flight are released later.
            Not callable from inside a pass's record().
            name:    the name the pass was added under.
            Returns: an error if there is no such pass or the graph is recording.
        */
        [[nodiscard]] Status removePass(const std::string& name);

        /*
            Calls the pass's describe() again, for when what it declares changed, e.g. a shadow map resolution or
            an attachment scale. The pass object is kept. Attachments are re-created on the next execute(), as
            after a window resize; GPU resources still used by frames in flight are released later.
            Not callable from inside a pass's record().
            name:    the name the pass was added under.
            Returns: an error if there is no such pass, the graph is recording, or the new description is invalid
                     (the old one then stays in effect).
        */
        [[nodiscard]] Status redescribePass(const std::string& name);

        void setPresentSource(const std::string& attachmentName);

        /*
            Captures the next frame as it reaches the window, e.g. for a screenshot. The pixels are copied on the
            GPU at the end of the next execute() that presents a frame, and handed to onReady from inside a later
            execute(), once the GPU finished that frame; nothing stalls. The image is RGBA8Srgb (sRGB-encoded,
            alpha as rendered), swapchain-sized, rows top to bottom with no padding; its pixels are only valid
            during the call. If the graph is destroyed first, onReady is never called.
            onReady: receives the captured image.
            Returns: an error if onReady is empty or a capture is already pending.
        */
        [[nodiscard]] Status readbackFrame(std::function<void(const ImageData&)> onReady);

        backend::RenderGraphImpl* impl() const;

        [[nodiscard]]FrameStats getFrameStats() const;

        /*
            GPU time of each pass that ran in the newest frame the GPU has finished, in execution order.
            Empty when the device has no timestamp support or no frame has finished yet. Passes overlap on the
            GPU, so the times need not add up to FrameStats::gpuFrameMs. Only the first 64 passes are timed.
            Returns: the timings, valid until the next execute().
        */
        [[nodiscard]] const std::vector<PassTiming>& getPassTimings() const;

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