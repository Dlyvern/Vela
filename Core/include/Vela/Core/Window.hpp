#ifndef VELA_CORE_WINDOW_HPP
#define VELA_CORE_WINDOW_HPP

#include "Vela/Core/IWindowBackend.hpp"
#include "Vela/Core/WindowTypes.hpp"
#include "Vela/Result.hpp"

#include <cstdint>
#include <vector>

namespace vela::core
{
    class Window
    {
    public:
        static Result<Window> create(const WindowPreferences& windowPreferences);
        static Result<Window> create(IWindowBackend& windowBackend, const WindowPreferences& windowPreferences);

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) noexcept;
        Window& operator=(Window&&) noexcept;

        bool isOpen() const;
        void close();

        IWindowBackend& getWindowBackend();
        void* getNativeHandle() const;

        void pollEvents();
        void setTitle(const std::string& title);

        void getSize(int& width, int& height) const;
        void getFramebufferSize(int& width, int& height) const;

        static constexpr uint32_t k_currentMonitor{0xFFFFFFFFu};

        void setMode(WindowMode mode, uint32_t monitorIndex = k_currentMonitor);
        WindowMode getMode() const;

        std::vector<MonitorInfo> getMonitors() const;
        [[nodiscard]] uint32_t getCurrentMonitor() const;
        [[nodiscard]] bool getPosition(int& x, int& y) const;

        /*
            Events gathered by the last pollEvents(), oldest first. The span points into the window's queue:
            it is invalidated by the next pollEvents() or popEvent() call. pollEvents() discards events nobody
            read. Use either events() (read all, keep them) or popEvent() (consume one by one), not both.
        */
        [[nodiscard]] std::span<const Event> events() const;

        /*
            Removes the oldest event from the queue and copies it into event. O(n) in the queued events,
            which is fine for one frame's worth.
            event:   receives the event.
            Returns: false when the queue is empty (event is unchanged).
        */
        [[nodiscard]] bool popEvent(Event& event) const;

        [[nodiscard]] bool isKeyDown(Key key) const;
        [[nodiscard]] bool isMouseButtonDown(MouseButton button) const;
        void getCursorPosition(double& x, double& y) const;

        void setCursorMode(CursorMode mode);
        void setCursorShape(CursorShape shape);

        /*
            Reports mouse movement without the OS's pointer acceleration and scaling while the cursor is
            CursorMode::Disabled, which is what mouse-look wants. Off by default. Has no effect in other cursor modes.
            enabled: true to turn raw motion on, false to turn it off.
            Returns: an error if the platform or window backend does not support raw motion (the setting is unchanged).
        */
        [[nodiscard]] Status setRawMouseMotion(bool enabled);

        [[nodiscard]] std::vector<int> getConnectedGamepads() const;
        [[nodiscard]] bool isGamepadConnected(int gamepadId) const;
        [[nodiscard]] std::string getGamepadName(int gamepadId) const;
        [[nodiscard]] bool isGamepadButtonDown(int gamepadId, GamepadButton button) const;
        [[nodiscard]] float getGamepadAxisLeftX(int gamepadId) const;
        [[nodiscard]] float getGamepadAxisLeftY(int gamepadId) const;
        [[nodiscard]] float getGamepadAxisRightX(int gamepadId) const;
        [[nodiscard]] float getGamepadAxisRightY(int gamepadId) const;

        ~Window();

    private:
        Window(IWindowBackend& windowBackend, const WindowPreferences& windowPreferences);

        void destroy();

        IWindowBackend* m_windowBackend{nullptr};
        void* m_nativeWindow{nullptr};
        WindowMode m_mode{WindowMode::Windowed};
    };
} //namespace vela::core

#endif //VELA_CORE_WINDOW_HPP
