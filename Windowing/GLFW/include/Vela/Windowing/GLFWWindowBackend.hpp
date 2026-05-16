#ifndef VELA_WINDOWING_GLFW_WINDOW_BACKEND_HPP
#define VELA_WINDOWING_GLFW_WINDOW_BACKEND_HPP

#include "Vela/Core/IWindowBackend.hpp"

namespace vela::windowing
{
    class GLFWWindowBackend : public core::IWindowBackend
    {
    public:
        GLFWWindowBackend();
        std::vector<const char*> requiredInstanceExtensions() const override;
        void* createSurface(void* instance, void* nativeWindowHandle) override;
        void* createNativeWindow(int w, int h, const std::string& title) override;
        void destroyNativeWindow(void* window) override;
        bool isOpen(void* window) const override;
        void pollEvents() override;
        void waitEvents() override;

        ~GLFWWindowBackend();
    private:
        static void glfwErrorCallback(int errorCode, const char* description);
    };
} //namespace vela::core

#endif //VELA_WINDOWING_GLFW_WINDOW_BACKEND_HPP