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

        void setMode(WindowMode mode, uint32_t monitorIndex = 0);
        WindowMode getMode() const;

        std::vector<MonitorInfo> monitors() const;

        [[nodiscard]] std::span<const Event> events() const;
        [[nodiscard]] bool popEvent(Event& event) const;

        [[nodiscard]] bool isKeyDown(Key key) const;
        [[nodiscard]] bool isMouseButtonDown(MouseButton button) const;
        void getCursorPosition(double& x, double& y) const;

        void setCursorMode(CursorMode mode);
        void setCursorShape(CursorShape shape);

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
