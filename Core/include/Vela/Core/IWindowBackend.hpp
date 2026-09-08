#ifndef VELA_CORE_WINDOW_BACKEND_IWINDOW_BACKEND_HPP
#define VELA_CORE_WINDOW_BACKEND_IWINDOW_BACKEND_HPP

#include "WindowTypes.hpp"
#include "Input.hpp"

#include <vector>
#include <string>
#include <span>

namespace vela::core
{
    class IWindowBackend
    {
    public:

        virtual void getFramebufferSize(void* nativeWindow, int& width, int& height) = 0;
        virtual void getWindowSize(void* nativeWindow, int& width, int& height) = 0;
        virtual void setTitle(void* nativeWindow, const std::string& title) = 0;
        virtual std::vector<const char*> requiredInstanceExtensions() const = 0;
        virtual void* createSurface(void* instance, void* nativeWindowHandle) = 0;
        virtual void* createNativeWindow(const WindowPreferences& windowPreferences) = 0;
        virtual void destroyNativeWindow(void* window) = 0;
        virtual bool isOpen(void* window) const = 0;
        virtual void pollEvents(void* nativeWindowHandle = nullptr) = 0; //? Maybe passing void* nativeWindowHandle here is weird
        virtual void waitEvents() = 0;
        virtual void close(void* nativeWindow) = 0;
        virtual void setMode(void* nativeWindow, WindowMode mode, uint32_t monitorIndex) = 0;
        virtual std::vector<MonitorInfo> monitors() const = 0;

        //Input
        // -----------------
        virtual std::span<const Event> events(void* nativeWindow) const = 0;
        virtual bool popEvent(void* nativeWindow, core::Event& event) = 0;
        virtual bool isKeyDown(void* nativeWindow, Key key) const = 0;

        //Mouse
        virtual bool isMouseButtonDown(void* nativeWindow, MouseButton button) const = 0;
        virtual void getCursorPosition(void* nativeWindow, double& x, double& y) const = 0;
        virtual void setCursorMode(void* nativeWindow, CursorMode mode) = 0;

        //TODO If a controller isn't detected properly, calling glfwUpdateGamepadMappings can add support for unmapped hardware
        //Gamepad
        virtual bool isGamepadButtonDown(int jid, GamepadButton button) const = 0;
        virtual float getGamepadAxisLeftX(int jid) const = 0;
        virtual float getGamepadAxisLeftY(int jid) const = 0;
        virtual float getGamepadAxisRightX(int jid) const = 0;
        virtual float getGamepadAxisRightY(int jid) const = 0;
        virtual bool isGamepadConnected(int jid) const = 0;
        virtual std::string getGamepadName(int jid) const = 0;
        virtual std::vector<int> getConnectedGamepads() const = 0;
        // -----------------
        
        virtual ~IWindowBackend() = default;
    };

    IWindowBackend& platformWindowBackend();
} //namespace vela::core

#endif //VELA_CORE_WINDOW_BACKEND_IWINDOW_BACKEND_HPP
