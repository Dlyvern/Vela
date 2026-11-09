#ifndef VELA_CORE_CONTEXT_HPP
#define VELA_CORE_CONTEXT_HPP

#include <memory>
#include <vector>

#include "Vela/Core/IWindowBackend.hpp"
#include "Vela/Core/ContextPreferences.hpp"
#include "Vela/Result.hpp"
#include "Vela/Core/MemoryStats.hpp"
#include "Vela/Core/DeviceInfo.hpp"

namespace vela::graphics
{
    class Mesh;
    class Material;
} //namespace vela::graphics

namespace vela::core
{
    class Window;
} //namespace vela::core

namespace vela::backend
{
    class ContextImpl;
} // namespace vela::core

namespace vela::core
{
    class Context
    {
    public:
        static Result<Context> create(Window& window, const ContextPreferences& contextPreferences = {});

        [[nodiscard]] Status attach(Window& window);
        [[nodiscard]] MemoryStats getMemoryStats() const;
        [[nodiscard]] const DeviceInfo& getDeviceInfo() const;
        [[nodiscard]] SwapchainInfo getSwapchainInfo() const;

        // Present modes the attached window's surface supports, as VSync values. VSync::On is always one of them.
        // Returns: the supported modes, or only VSync::On before attach().
        [[nodiscard]] std::vector<VSync> getSupportedVSync() const;

        /*
            Changes how frames are presented without recreating the context. The next RenderGraph::execute()
            recreates the swapchain; attachments are kept when the window size did not change.
            An unsupported mode falls back as described on VSync; getSwapchainInfo() reports the result.
            vsync:   the requested mode.
            Returns: an error before attach().
        */
        [[nodiscard]] Status setVSync(VSync vsync);
        [[nodiscard]] uint32_t getValidationMessageCount() const;

        void waitIdle();

        /*
            Waits for the GPU to finish all submitted work, then frees the GPU memory of every resource already
            destroyed (normally held back until the frames in flight that may use it have finished) and of
            finished texture uploads' staging buffers. Use it between heavy unloads and loads, e.g. a level
            switch, so the old memory is gone before the new is allocated. Stalls the GPU; not for every frame.
            Returns: an error before attach() or if waiting for the device failed (nothing is freed then).
        */
        [[nodiscard]] Status releaseDestroyed();

        ~Context();
        Context(Context&&) noexcept;
        Context& operator=(Context&&) noexcept;
        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;
        backend::ContextImpl* impl();
    private:
        Context(IWindowBackend& windowBackend, const ContextPreferences& contextPreferences);

        std::unique_ptr<backend::ContextImpl> m_contextImpl{nullptr};
    };
} //namespace vela::core


#endif //VELA_CORE_CONTEXT_HPP