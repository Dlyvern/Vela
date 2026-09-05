#ifndef VELA_WINDOWING_DESKTOP_WINDOW_BACKEND_HPP
#define VELA_WINDOWING_DESKTOP_WINDOW_BACKEND_HPP

#include "Vela/Core/IWindowBackend.hpp"

#include <unordered_map>

namespace vela::windowing
{
    class DesktopWindowBackend : public core::IWindowBackend
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
        void pollEvents() override;
        void waitEvents() override;
        void close(void* nativeWindow) override;
        void setMode(void* nativeWindow, core::WindowMode mode, uint32_t monitorIndex) override;
        std::vector<core::MonitorInfo> monitors() const override;

        ~DesktopWindowBackend();
    private:
        struct WindowedRect
        {
            int x{0};
            int y{0};
            int width{0};
            int height{0};
        };

        static void glfwErrorCallback(int errorCode, const char* description);

        std::unordered_map<void*, WindowedRect> m_windowedRects;
    };
} //namespace vela::core

#endif //VELA_WINDOWING_DESKTOP_WINDOW_BACKEND_HPP
