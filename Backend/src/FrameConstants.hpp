#ifndef VELA_BACKEND_FRAME_CONSTANTS_HPP
#define VELA_BACKEND_FRAME_CONSTANTS_HPP

#include <cstdint>

namespace vela::backend
{
    class FrameConstants
    {
    public:
        /*
            How many frames the CPU may run ahead of the GPU. One would mean the
            CPU blocks on the previous frame before it can record the next one,
            so every GPU/present stall lands directly in the frame time
        */
        static constexpr uint32_t k_framesInFlight{2};
    };
} //namespace vela::backend

#endif //VELA_BACKEND_FRAME_CONSTANTS_HPP