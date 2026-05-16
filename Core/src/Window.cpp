#include "Vela/Core/Window.hpp"

namespace vela::core
{
    Window::Window(IWindowBackend& windowBackend, int w, int h, const std::string& title) : m_windowBackend(windowBackend)
    {
        m_nativeWindow = m_windowBackend.createNativeWindow(w, h, title);
    }

    void Window::pollEvents()
    {
        m_windowBackend.pollEvents();
    }

    void* const Window::getNativeHandle()
    {
        return m_nativeWindow;
    }

    bool Window::isOpen() const
    {
        return m_windowBackend.isOpen(m_nativeWindow);
    }

    IWindowBackend& Window::getWindowBackend()
    {
        return m_windowBackend;
    }

    Window::~Window()
    {
        m_windowBackend.destroyNativeWindow(m_nativeWindow);
    }

} // namespace vela::core
