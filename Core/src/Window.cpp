#include "Vela/Core/Window.hpp"

#include <utility>

namespace vela::core
{
    Window::Window(IWindowBackend& windowBackend, const WindowPreferences& windowPreferences)
        : m_windowBackend(&windowBackend), m_mode(windowPreferences.mode)
    {
        m_nativeWindow = m_windowBackend->createNativeWindow(windowPreferences);
    }

    Window::Window(Window&& other) noexcept
        : m_windowBackend(other.m_windowBackend), m_nativeWindow(other.m_nativeWindow), m_mode(other.m_mode)
    {
        other.m_nativeWindow = nullptr;
    }

    Window& Window::operator=(Window&& other) noexcept
    {
        if (this == &other)
            return *this;

        destroy();

        m_windowBackend = other.m_windowBackend;
        m_nativeWindow = other.m_nativeWindow;
        m_mode = other.m_mode;

        other.m_nativeWindow = nullptr;

        return *this;
    }

    void Window::pollEvents()
    {
        m_windowBackend->pollEvents();
    }

    void Window::setTitle(const std::string& title)
    {
        m_windowBackend->setTitle(m_nativeWindow, title);
    }

    void* Window::getNativeHandle() const
    {
        return m_nativeWindow;
    }

    bool Window::isOpen() const
    {
        return m_nativeWindow != nullptr && m_windowBackend->isOpen(m_nativeWindow);
    }

    void Window::close()
    {
        m_windowBackend->close(m_nativeWindow);
    }

    void Window::getSize(int& width, int& height) const
    {
        m_windowBackend->getWindowSize(m_nativeWindow, width, height);
    }

    void Window::getFramebufferSize(int& width, int& height) const
    {
        m_windowBackend->getFramebufferSize(m_nativeWindow, width, height);
    }

    void Window::setMode(WindowMode mode, uint32_t monitorIndex)
    {
        m_windowBackend->setMode(m_nativeWindow, mode, monitorIndex);
        m_mode = mode;
    }

    WindowMode Window::getMode() const
    {
        return m_mode;
    }

    std::vector<MonitorInfo> Window::monitors() const
    {
        return m_windowBackend->monitors();
    }

    IWindowBackend& Window::getWindowBackend()
    {
        return *m_windowBackend;
    }

    void Window::destroy()
    {
        if (m_nativeWindow == nullptr)
            return;

        m_windowBackend->destroyNativeWindow(m_nativeWindow);
        m_nativeWindow = nullptr;
    }

    Window::~Window()
    {
        destroy();
    }
} // namespace vela::core
