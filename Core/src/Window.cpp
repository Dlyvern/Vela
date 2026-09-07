#include "Vela/Core/Window.hpp"

namespace vela::core
{
    Window::Window(IWindowBackend& windowBackend, const WindowPreferences& windowPreferences)
        : m_windowBackend(&windowBackend), m_mode(windowPreferences.mode)
    {
        m_nativeWindow = m_windowBackend->createNativeWindow(windowPreferences);
    }

    Result<Window> Window::create(const WindowPreferences& windowPreferences)
    {
        return create(platformWindowBackend(), windowPreferences);
    }

    Result<Window> Window::create(IWindowBackend& windowBackend, const WindowPreferences& windowPreferences)
    {
        try
        {
            return Window(windowBackend, windowPreferences);
        }
        catch (const std::exception& error)
        {
            return Error{ErrorCode::WindowCreationFailed, error.what()};
        }
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

    std::span<const Event> Window::events() const
    {
        return m_windowBackend->events(m_nativeWindow);
    }

    bool Window::isKeyDown(Key key) const
    {
        return m_windowBackend->isKeyDown(m_nativeWindow, key);
    }

    bool Window::isMouseButtonDown(MouseButton button) const
    {
        return m_windowBackend->isMouseButtonDown(m_nativeWindow, button);
    }

    void Window::getCursorPosition(double& x, double& y) const
    {
        m_windowBackend->getCursorPosition(m_nativeWindow, x, y);
    }

    void Window::setCursorMode(CursorMode mode)
    {
        m_windowBackend->setCursorMode(m_nativeWindow, mode);
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
