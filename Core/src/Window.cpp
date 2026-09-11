#include "Vela/Core/Window.hpp"

#include <algorithm>

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

    bool Window::popEvent(Event& event) const
    {
        return m_windowBackend->popEvent(m_nativeWindow, event);
    }

    std::vector<int> Window::getConnectedGamepads() const
    {
        return m_windowBackend->getConnectedGamepads();
    }

    bool Window::isGamepadConnected(int gamepadId) const
    {
        return m_windowBackend->isGamepadConnected(gamepadId);
    }

    std::string Window::getGamepadName(int gamepadId) const
    {
        return m_windowBackend->getGamepadName(gamepadId);
    }

    bool Window::isGamepadButtonDown(int gamepadId, GamepadButton button) const
    {
        return m_windowBackend->isGamepadButtonDown(gamepadId, button);
    }

    float Window::getGamepadAxisLeftX(int gamepadId) const
    {
        return m_windowBackend->getGamepadAxisLeftX(gamepadId);
    }

    float Window::getGamepadAxisLeftY(int gamepadId) const
    {
        return m_windowBackend->getGamepadAxisLeftY(gamepadId);
    }

    float Window::getGamepadAxisRightX(int gamepadId) const
    {
        return m_windowBackend->getGamepadAxisRightX(gamepadId);
    }

    float Window::getGamepadAxisRightY(int gamepadId) const
    {
        return m_windowBackend->getGamepadAxisRightY(gamepadId);
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

    void Window::setCursorShape(CursorShape shape)
    {
        m_windowBackend->setCursorShape(m_nativeWindow, shape);
    }

    void Window::setCursorMode(CursorMode mode)
    {
        m_windowBackend->setCursorMode(m_nativeWindow, mode);
    }

    void Window::pollEvents()
    {
        m_windowBackend->pollEvents(m_nativeWindow);
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

    bool Window::getPosition(int& x, int& y) const
    {
        return m_windowBackend->getWindowPosition(m_nativeWindow, x, y);
    }

    uint32_t Window::getCurrentMonitor() const
    {
        const std::vector<MonitorInfo> monitors = getMonitors();

        if (monitors.empty())
            return 0;

        int windowX = 0;
        int windowY = 0;

        if (!getPosition(windowX, windowY))
            return 0;

        int windowWidth = 0;
        int windowHeight = 0;

        getSize(windowWidth, windowHeight);

        uint32_t bestMonitor = 0;
        long bestOverlap = -1;

        for (size_t index = 0; index < monitors.size(); ++index)
        {
            const MonitorInfo& monitor = monitors[index];

            const int overlapWidth = std::min(windowX + windowWidth, monitor.x + monitor.width)
                                   - std::max(windowX, monitor.x);

            const int overlapHeight = std::min(windowY + windowHeight, monitor.y + monitor.height)
                                    - std::max(windowY, monitor.y);

            const long overlap = static_cast<long>(std::max(0, overlapWidth))
                               * static_cast<long>(std::max(0, overlapHeight));

            if (overlap > bestOverlap)
            {
                bestOverlap = overlap;
                bestMonitor = static_cast<uint32_t>(index);
            }
        }

        return bestMonitor;
    }

    void Window::setMode(WindowMode mode, uint32_t monitorIndex)
    {
        if (monitorIndex == k_currentMonitor)
            monitorIndex = getCurrentMonitor();

        m_windowBackend->setMode(m_nativeWindow, mode, monitorIndex);
        m_mode = mode;
    }

    WindowMode Window::getMode() const
    {
        return m_mode;
    }

    std::vector<MonitorInfo> Window::getMonitors() const
    {
        return m_windowBackend->getMonitors();
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
