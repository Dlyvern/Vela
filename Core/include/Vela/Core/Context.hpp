#ifndef VELA_CORE_CONTEXT_HPP
#define VELA_CORE_CONTEXT_HPP

#include <memory>

#include "Vela/Core/IWindowBackend.hpp"
#include "Vela/Core/ContextPreferences.hpp"

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
        Context(IWindowBackend& windowBackend, const ContextPreferences& contextPreferences = {});
        void createSurfaceFor(Window& window);

        void waitIdle();

        ~Context();
        Context(Context&&) noexcept;
        Context& operator=(Context&&) noexcept;
        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;
        backend::ContextImpl* impl();
    private:
        std::unique_ptr<backend::ContextImpl> m_contextImpl{nullptr};
    };
} //namespace vela::core


#endif //VELA_CORE_CONTEXT_HPP