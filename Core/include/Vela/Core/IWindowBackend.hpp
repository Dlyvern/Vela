#ifndef VELA_CORE_WINDOW_BACKEND_IWINDOW_BACKEND_HPP
#define VELA_CORE_WINDOW_BACKEND_IWINDOW_BACKEND_HPP

#include <vector>
#include <string>

namespace vela::core
{
    class IWindowBackend
    {
    public:
        virtual std::vector<const char*> requiredInstanceExtensions() const = 0;
        virtual void* createSurface(void* instance, void* nativeWindowHandle) = 0;
        virtual void* createNativeWindow(int w, int h, const std::string& title) = 0;
        virtual void destroyNativeWindow(void* window) = 0;
        virtual bool isOpen(void* window) const = 0;
        virtual void pollEvents() = 0;
        virtual void waitEvents() = 0;
        virtual ~IWindowBackend() = default;
    };
} //namespace vela::core

#endif //VELA_CORE_WINDOW_BACKEND_IWINDOW_BACKEND_HPP