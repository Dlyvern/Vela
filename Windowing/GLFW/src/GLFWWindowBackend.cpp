#include "Vela/Windowing/GLFWWindowBackend.hpp"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <stdexcept>
#include <iostream>

namespace vela::windowing
{
    GLFWWindowBackend::GLFWWindowBackend()
    {
        glfwSetErrorCallback(&GLFWWindowBackend::glfwErrorCallback);

        if (!glfwInit())
            throw std::runtime_error("Failed to initialize GLFW");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
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

    void* GLFWWindowBackend::createNativeWindow(int w, int h, const std::string& title)
    {
        auto window = glfwCreateWindow(w, h, title.c_str(), nullptr, nullptr);
        return window;
    }

    void GLFWWindowBackend::destroyNativeWindow(void* window) 
    {
        auto glfwWindow = static_cast<GLFWwindow*>(window);
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