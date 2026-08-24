#include "Vela/Windowing/GLFWWindowBackend.hpp"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <stdexcept>
#include <iostream>

namespace
{
    bool windowPositionSupported()
    {
        return glfwGetPlatform() != GLFW_PLATFORM_WAYLAND;
    }
} //namespace

namespace vela::windowing
{
    GLFWWindowBackend::GLFWWindowBackend()
    {
        glfwSetErrorCallback(&GLFWWindowBackend::glfwErrorCallback);

        if (!glfwInit())
            throw std::runtime_error("Failed to initialize GLFW");
    }

    GLFWmonitor* GLFWWindowBackend::monitorAt(uint32_t monitorIndex)
    {
        int count{0};
        GLFWmonitor** monitors = glfwGetMonitors(&count);

        if (monitors == nullptr || count <= 0)
            return nullptr;

        if (monitorIndex >= static_cast<uint32_t>(count))
            return glfwGetPrimaryMonitor();

        return monitors[monitorIndex];
    }

    void GLFWWindowBackend::getFramebufferSize(void* nativeWindow, int& width, int& height)
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindow);
        glfwGetFramebufferSize(glfwWindow, &width, &height);
    }

    void GLFWWindowBackend::getWindowSize(void* nativeWindow, int& width, int& height)
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindow);
        glfwGetWindowSize(glfwWindow, &width, &height);
    }

    void GLFWWindowBackend::setTitle(void* nativeWindow, const std::string& title)
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindow);
        glfwSetWindowTitle(glfwWindow, title.c_str());
    }

    void GLFWWindowBackend::glfwErrorCallback(int errorCode, const char* description)
    {
        std::cerr << "[GLFWWindowBackend] Code: " << errorCode
              << " Description: "
              << description
              << '\n';
    }

    void GLFWWindowBackend::pollEvents()
    {
        glfwPollEvents();
    }

    void GLFWWindowBackend::waitEvents()
    {
        glfwWaitEvents();
    }

    std::vector<const char*> GLFWWindowBackend::requiredInstanceExtensions() const
    {
        uint32_t count = 0;
        const char** exts = glfwGetRequiredInstanceExtensions(&count);
        return std::vector<const char*>(exts, exts + count);
    }

    void* GLFWWindowBackend::createSurface(void* instance, void* nativeWindowHandle)
    {
        auto vkInstance = static_cast<VkInstance>(instance);
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindowHandle);
        VkSurfaceKHR surface;

        if(VkResult result = glfwCreateWindowSurface(vkInstance, glfwWindow, nullptr, &surface); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create GLFW surface");

        return surface;
    }

    void GLFWWindowBackend::close(void* nativeWindow)
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindow);
        glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
    }

    std::vector<core::MonitorInfo> GLFWWindowBackend::monitors() const
    {
        int count{0};
        GLFWmonitor** monitors = glfwGetMonitors(&count);

        std::vector<core::MonitorInfo> result;

        if (monitors == nullptr || count <= 0)
            return result;

        result.reserve(static_cast<size_t>(count));

        for (int index = 0; index < count; ++index)
        {
            core::MonitorInfo info;

            const char* name = glfwGetMonitorName(monitors[index]);
            info.name = name != nullptr ? name : "";

            glfwGetMonitorPos(monitors[index], &info.x, &info.y);

            if (const GLFWvidmode* videoMode = glfwGetVideoMode(monitors[index]); videoMode != nullptr)
            {
                info.width = videoMode->width;
                info.height = videoMode->height;
                info.refreshRate = videoMode->refreshRate;
            }

            result.push_back(std::move(info));
        }

        return result;
    }

    void* GLFWWindowBackend::createNativeWindow(const core::WindowPreferences& windowPreferences)
    {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, windowPreferences.resizable ? GLFW_TRUE : GLFW_FALSE);

        GLFWmonitor* monitor = monitorAt(windowPreferences.monitorIndex);

        const bool fullscreen = windowPreferences.mode != core::WindowMode::eWINDOWED && monitor != nullptr;

        GLFWwindow* window = nullptr;

        if (!fullscreen)
        {
            window = glfwCreateWindow(windowPreferences.width, windowPreferences.height,
                windowPreferences.title.c_str(), nullptr, nullptr);
        }
        else if (windowPreferences.mode == core::WindowMode::eBORDERLESS_FULLSCREEN)
        {
            const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);

            glfwWindowHint(GLFW_RED_BITS, videoMode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, videoMode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, videoMode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, videoMode->refreshRate);

            window = glfwCreateWindow(videoMode->width, videoMode->height,
                windowPreferences.title.c_str(), monitor, nullptr);
        }
        else
        {
            window = glfwCreateWindow(windowPreferences.width, windowPreferences.height,
                windowPreferences.title.c_str(), monitor, nullptr);
        }

        if (window == nullptr)
            throw std::runtime_error("Failed to create window");

        WindowedRect rect;
        rect.width = windowPreferences.width;
        rect.height = windowPreferences.height;

        if (!fullscreen)
        {
            if (windowPositionSupported())
                glfwGetWindowPos(window, &rect.x, &rect.y);

            glfwGetWindowSize(window, &rect.width, &rect.height);
        }
        else
        {
            int monitorX{0};
            int monitorY{0};
            int monitorWidth{0};
            int monitorHeight{0};
            glfwGetMonitorWorkarea(monitor, &monitorX, &monitorY, &monitorWidth, &monitorHeight);

            rect.x = monitorX + (monitorWidth - rect.width) / 2;
            rect.y = monitorY + (monitorHeight - rect.height) / 2;
        }

        m_windowedRects[window] = rect;

        return window;
    }

    void GLFWWindowBackend::setMode(void* nativeWindow, core::WindowMode mode, uint32_t monitorIndex)
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindow);

        if (glfwGetWindowMonitor(glfwWindow) == nullptr)
        {
            WindowedRect& saved = m_windowedRects[nativeWindow];

            if (windowPositionSupported())
                glfwGetWindowPos(glfwWindow, &saved.x, &saved.y);

            glfwGetWindowSize(glfwWindow, &saved.width, &saved.height);
        }

        GLFWmonitor* monitor = monitorAt(monitorIndex);
        const WindowedRect rect = m_windowedRects[nativeWindow];

        if (mode == core::WindowMode::eWINDOWED || monitor == nullptr)
        {
            glfwSetWindowMonitor(glfwWindow, nullptr, rect.x, rect.y, rect.width, rect.height, GLFW_DONT_CARE);
            return;
        }

        const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);

        if (mode == core::WindowMode::eBORDERLESS_FULLSCREEN)
        {
            glfwSetWindowMonitor(glfwWindow, monitor, 0, 0,
                videoMode->width, videoMode->height, videoMode->refreshRate);
        }
        else
        {
            glfwSetWindowMonitor(glfwWindow, monitor, 0, 0, rect.width, rect.height, GLFW_DONT_CARE);
        }
    }

    void GLFWWindowBackend::destroyNativeWindow(void* window)
    {
        auto glfwWindow = static_cast<GLFWwindow*>(window);

        m_windowedRects.erase(window);

        glfwDestroyWindow(glfwWindow);
    }

    bool GLFWWindowBackend::isOpen(void* window) const
    {
        auto glfwWindow = static_cast<GLFWwindow*>(window);

        return !glfwWindowShouldClose(glfwWindow);
    }

    GLFWWindowBackend::~GLFWWindowBackend()
    {
        glfwTerminate();
    }
} //namespace vela::windowing
