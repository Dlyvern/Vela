#ifndef VELA_CORE_CONTEXT_HPP
#define VELA_CORE_CONTEXT_HPP

#include <memory>

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
        [[nodiscard]] uint32_t getValidationMessageCount() const;

        void waitIdle();

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