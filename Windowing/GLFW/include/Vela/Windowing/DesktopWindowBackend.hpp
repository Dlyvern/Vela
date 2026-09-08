#ifndef VELA_WINDOWING_DESKTOP_WINDOW_BACKEND_HPP
#define VELA_WINDOWING_DESKTOP_WINDOW_BACKEND_HPP

#include "Vela/Core/IWindowBackend.hpp"
#include "Vela/Core/Input.hpp"

#include <array>
#include <unordered_map>

namespace vela::windowing
{
    class DesktopWindowBackend final : public core::IWindowBackend
    {
    public:
        DesktopWindowBackend();
        void getFramebufferSize(void* nativeWindow, int& width, int& height) override;
        void getWindowSize(void* nativeWindow, int& width, int& height) override;
        void setTitle(void* nativeWindow, const std::string& title) override;
        std::vector<const char*> requiredInstanceExtensions() const override;
        void* createSurface(void* instance, void* nativeWindowHandle) override;
        void* createNativeWindow(const core::WindowPreferences& windowPreferences) override;
        void destroyNativeWindow(void* window) override;
        bool isOpen(void* window) const override;
        void pollEvents(void* nativeWindowHandle = nullptr) override;
        void waitEvents() override;
        void close(void* nativeWindow) override;
        void setMode(void* nativeWindow, core::WindowMode mode, uint32_t monitorIndex) override;
        std::vector<core::MonitorInfo> monitors() const override;

        std::span<const core::Event> events(void* nativeWindow) const override;
        bool isKeyDown(void* nativeWindow, core::Key key) const override;
        bool isMouseButtonDown(void* nativeWindow, core::MouseButton button) const override;
        void getCursorPosition(void* nativeWindow, double& x, double& y) const override;
        void setCursorMode(void* nativeWindow, core::CursorMode mode) override;

        bool popEvent(void* nativeWindow, core::Event& event) override;

        bool isGamepadButtonDown(int jid, core::GamepadButton button) const override;
        float getGamepadAxisLeftX(int jid) const override;
        float getGamepadAxisLeftY(int jid) const override;
        float getGamepadAxisRightX(int jid) const override;
        float getGamepadAxisRightY(int jid) const override;
        bool isGamepadConnected(int jid) const override;
        std::vector<int> getConnectedGamepads() const override;
        std::string getGamepadName(int jid) const override;

        ~DesktopWindowBackend();
    private:
        struct WindowedRect
        {
            int x{0};
            int y{0};
            int width{0};
            int height{0};
        };

        void pollGamepadEvents(void* nativeWindowHandle);

        static void glfwErrorCallback(int errorCode, const char* description);

        std::unordered_map<void*, WindowedRect> m_windowedRects;
        std::unordered_map<void*, std::vector<core::Event>> m_events;

        struct GamepadState
        {
            bool isConnected{false};
            int id{0};
            bool firedAboutConnection{false};

            float prevGamepadRightAxisX{0.0f};
            float prevGamepadRightAxisY{0.0f};
            float prevGamepadLeftAxisX{0.0f};
            float prevGamepadLeftAxisY{0.0f};

            std::array<bool, static_cast<size_t>(core::GamepadButton::LAST)> prevButtons{};
        };

        void updateGamepadState(GamepadState& gamepadState);

        //TODO Should be fixed, no global or static state.
        static inline std::vector<GamepadState> m_gamepads;
    };
} //namespace vela::core

#endif //VELA_WINDOWING_DESKTOP_WINDOW_BACKEND_HPP