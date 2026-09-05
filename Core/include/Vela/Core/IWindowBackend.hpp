#ifndef VELA_CORE_WINDOW_BACKEND_IWINDOW_BACKEND_HPP
#define VELA_CORE_WINDOW_BACKEND_IWINDOW_BACKEND_HPP

#include "Vela/Core/WindowTypes.hpp"

#include <vector>
#include <string>

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
        virtual void pollEvents() = 0;
        virtual void waitEvents() = 0;
        virtual void close(void* nativeWindow) = 0;
        virtual void setMode(void* nativeWindow, WindowMode mode, uint32_t monitorIndex) = 0;
        virtual std::vector<MonitorInfo> monitors() const = 0;
        virtual ~IWindowBackend() = default;
    };

    IWindowBackend& platformWindowBackend();
} //namespace vela::core

#endif //VELA_CORE_WINDOW_BACKEND_IWINDOW_BACKEND_HPP
