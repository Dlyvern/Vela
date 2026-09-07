#ifndef VELA_GRAPHICS_FRAME_STATS_HPP
#define VELA_GRAPHICS_FRAME_STATS_HPP

namespace vela::graphics
{
    struct FrameStats
    {
        float cpuFrameMs{0.0f};
        float gpuFrameMs{0.0f};
        bool gpuTimingValid{false};
    };
} //namespace vela::graphics

#endif //VELA_GRAPHICS_FRAME_STATS_HPP