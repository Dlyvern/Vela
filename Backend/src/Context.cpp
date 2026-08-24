#include "Vela/Core/Context.hpp"
#include "ContextImpl.hpp"

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

    void Context::createSurfaceFor(Window& window)
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
        m_contextImpl->createDepthImage();
    }

    Context::Context(Context&&) noexcept = default;
    Context& Context::operator=(Context&&) noexcept = default;

    Context::~Context() = default;
    
} //namespace vela::core