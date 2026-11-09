#ifndef VELA_GRAPHICS_FRAME_STATS_HPP
#define VELA_GRAPHICS_FRAME_STATS_HPP

#include <string>

namespace vela::graphics
{
    // Timings of the most recent RenderGraph::execute(). GPU values are from the newest frame the GPU has finished.
    struct FrameStats
    {
        // Wall time of the whole execute() call: cpuWaitMs + cpuWorkMs
        float cpuFrameMs{0.0f};
        float gpuFrameMs{0.0f};
        bool gpuTimingValid{false};

        /*
            Part of cpuFrameMs spent blocked rather than working: waiting for the frame in flight that last
            used this frame's resources, and for vkAcquireNextImageKHR to hand out a swapchain image (where a
            vsync'd present mode usually waits). Large values mean the CPU is ahead of the GPU or the display
        */
        float cpuWaitMs{0.0f};

        // Part of cpuFrameMs spent recording passes, submitting and presenting
        float cpuWorkMs{0.0f};
    };

    // GPU time of one pass in the newest frame the GPU has finished. See RenderGraph::getPassTimings()
    struct PassTiming
    {
        std::string name;

        // From the pass's first command (including the barriers Vela records for it) to its last
        float gpuMs{0.0f};
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_FRAME_STATS_HPP
