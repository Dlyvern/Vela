#include "Vela/Core/Context.hpp"
#include "ContextImpl.hpp"

#include <stdexcept>

#include "Vela/Core/Window.hpp"
#include "MeshImpl.hpp"
#include "MaterialImpl.hpp"

namespace vela::core
{
    Context::Context(IWindowBackend& windowBackend, const ContextPreferences& contextPreferences)
    {
        m_contextImpl = std::make_unique<backend::ContextImpl>(windowBackend, contextPreferences);
    }

    backend::ContextImpl* Context::impl()
    {
        return m_contextImpl.get();
    }

    void Context::waitIdle()
    {
        m_contextImpl->waitIdle();
    }

    Result<Context> Context::create(Window& window, const ContextPreferences& contextPreferences)
    {
        try
        {
            return Context(window.getWindowBackend(), contextPreferences);
        }
        catch (const std::exception& error)
        {
            return Error{ErrorCode::DeviceCreationFailed, error.what()};
        }
    }

    MemoryStats Context::getMemoryStats() const
    {   
        return m_contextImpl->getMemoryStats();
    }

    Status Context::attach(Window& window)
    {
        try
        {
            auto surface = window.getWindowBackend().createSurface(m_contextImpl->getInstance(), window.getNativeHandle());
            m_contextImpl->setSurface(static_cast<VkSurfaceKHR>(surface));
            m_contextImpl->setNativeWindow(window.getNativeHandle());
            m_contextImpl->pickPhysicalDevice();
            m_contextImpl->createDevice();
            m_contextImpl->createAllocator();
            m_contextImpl->createSwapchain();
            m_contextImpl->createSwapchainImageViews();
            m_contextImpl->createCommandPool();
        }
        catch (const std::exception& error)
        {
            return Error{ErrorCode::SurfaceCreationFailed, error.what()};
        }

        return {};
    }

    Context::Context(Context&&) noexcept = default;
    Context& Context::operator=(Context&&) noexcept = default;

    Context::~Context() = default;
    
} //namespace vela::core