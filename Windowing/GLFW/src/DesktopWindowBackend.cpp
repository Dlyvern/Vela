#include "Vela/Windowing/DesktopWindowBackend.hpp"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <stdexcept>
#include <iostream>

namespace
{
    GLFWmonitor* monitorAt(uint32_t monitorIndex)
    {
        int count{0};
        GLFWmonitor** monitors = glfwGetMonitors(&count);

        if (monitors == nullptr || count <= 0)
            return nullptr;

        if (monitorIndex >= static_cast<uint32_t>(count))
            return glfwGetPrimaryMonitor();

        return monitors[monitorIndex];
    }

    bool windowPositionSupported()
    {
        return glfwGetPlatform() != GLFW_PLATFORM_WAYLAND;
    }

    int toGLFWKey(vela::core::Key key)
    {
        switch (key) 
        {
            case vela::core::Key::Unknown: return GLFW_KEY_UNKNOWN;

            case vela::core::Key::A: return GLFW_KEY_A;
            case vela::core::Key::B: return GLFW_KEY_B;
            case vela::core::Key::C: return GLFW_KEY_C;
            case vela::core::Key::D: return GLFW_KEY_D;
            case vela::core::Key::E: return GLFW_KEY_E;
            case vela::core::Key::F: return GLFW_KEY_F;
            case vela::core::Key::G: return GLFW_KEY_G;
            case vela::core::Key::H: return GLFW_KEY_H;
            case vela::core::Key::I: return GLFW_KEY_I;
            case vela::core::Key::J: return GLFW_KEY_J;
            case vela::core::Key::K: return GLFW_KEY_K;
            case vela::core::Key::L: return GLFW_KEY_L;
            case vela::core::Key::M: return GLFW_KEY_M;
            case vela::core::Key::N: return GLFW_KEY_N;
            case vela::core::Key::O: return GLFW_KEY_O;
            case vela::core::Key::P: return GLFW_KEY_P;
            case vela::core::Key::Q: return GLFW_KEY_Q;
            case vela::core::Key::R: return GLFW_KEY_R;
            case vela::core::Key::S: return GLFW_KEY_S;
            case vela::core::Key::T: return GLFW_KEY_T;
            case vela::core::Key::U: return GLFW_KEY_U;
            case vela::core::Key::V: return GLFW_KEY_V;
            case vela::core::Key::W: return GLFW_KEY_W;
            case vela::core::Key::X: return GLFW_KEY_X;
            case vela::core::Key::Y: return GLFW_KEY_Y;
            case vela::core::Key::Z: return GLFW_KEY_Z;
                
            case vela::core::Key::Num0: return GLFW_KEY_KP_0;
            case vela::core::Key::Num1: return GLFW_KEY_KP_1;
            case vela::core::Key::Num2: return GLFW_KEY_KP_2;
            case vela::core::Key::Num3: return GLFW_KEY_KP_3;
            case vela::core::Key::Num4: return GLFW_KEY_KP_4;
            case vela::core::Key::Num5: return GLFW_KEY_KP_5;
            case vela::core::Key::Num6: return GLFW_KEY_KP_6;
            case vela::core::Key::Num7: return GLFW_KEY_KP_7;
            case vela::core::Key::Num8: return GLFW_KEY_KP_8;
            case vela::core::Key::Num9: return GLFW_KEY_KP_9;

            case vela::core::Key::Escape: return GLFW_KEY_ESCAPE;
            case vela::core::Key::Enter: return GLFW_KEY_ENTER;
            case vela::core::Key::Tab: return GLFW_KEY_TAB;
            case vela::core::Key::Backspace: return GLFW_KEY_BACKSPACE;
            case vela::core::Key::Space: return GLFW_KEY_SPACE;

            case vela::core::Key::Left: return GLFW_KEY_LEFT;
            case vela::core::Key::Right: return GLFW_KEY_RIGHT;
            case vela::core::Key::Up: return GLFW_KEY_UP;
            case vela::core::Key::Down: return GLFW_KEY_DOWN;

            case vela::core::Key::LeftShift: return GLFW_KEY_LEFT_SHIFT;
            case vela::core::Key::LeftControl: return GLFW_KEY_LEFT_CONTROL;
            case vela::core::Key::LeftAlt: return GLFW_KEY_LEFT_ALT;

            case vela::core::Key::RightShift: return GLFW_KEY_RIGHT_SHIFT;
            case vela::core::Key::RightControl: return GLFW_KEY_RIGHT_CONTROL;
            case vela::core::Key::RightAlt: return GLFW_KEY_RIGHT_ALT;

            case vela::core::Key::F1: return GLFW_KEY_F1;
            case vela::core::Key::F2: return GLFW_KEY_F2;
            case vela::core::Key::F3: return GLFW_KEY_F3;
            case vela::core::Key::F4: return GLFW_KEY_F4;
            case vela::core::Key::F5: return GLFW_KEY_F5;
            case vela::core::Key::F6: return GLFW_KEY_F6;
            case vela::core::Key::F7: return GLFW_KEY_F7;
            case vela::core::Key::F8: return GLFW_KEY_F8;
            case vela::core::Key::F9: return GLFW_KEY_F9;
            case vela::core::Key::F10: return GLFW_KEY_F10;
            case vela::core::Key::F11: return GLFW_KEY_F11;
            case vela::core::Key::F12: return GLFW_KEY_F12;
        }

        return GLFW_KEY_UNKNOWN;
    }

    vela::core::Key fromGLFWKey(int key)
    {
        switch (key)
        {
            case GLFW_KEY_UNKNOWN: return vela::core::Key::Unknown;

            case GLFW_KEY_A: return vela::core::Key::A;
            case GLFW_KEY_B: return vela::core::Key::B;
            case GLFW_KEY_C: return vela::core::Key::C;
            case GLFW_KEY_D: return vela::core::Key::D;
            case GLFW_KEY_E: return vela::core::Key::E;
            case GLFW_KEY_F: return vela::core::Key::F;
            case GLFW_KEY_G: return vela::core::Key::G;
            case GLFW_KEY_H: return vela::core::Key::H;
            case GLFW_KEY_I: return vela::core::Key::I;
            case GLFW_KEY_J: return vela::core::Key::J;
            case GLFW_KEY_K: return vela::core::Key::K;
            case GLFW_KEY_L: return vela::core::Key::L;
            case GLFW_KEY_M: return vela::core::Key::M;
            case GLFW_KEY_N: return vela::core::Key::N;
            case GLFW_KEY_O: return vela::core::Key::O;
            case GLFW_KEY_P: return vela::core::Key::P;
            case GLFW_KEY_Q: return vela::core::Key::Q;
            case GLFW_KEY_R: return vela::core::Key::R;
            case GLFW_KEY_S: return vela::core::Key::S;
            case GLFW_KEY_T: return vela::core::Key::T;
            case GLFW_KEY_U: return vela::core::Key::U;
            case GLFW_KEY_V: return vela::core::Key::V;
            case GLFW_KEY_W: return vela::core::Key::W;
            case GLFW_KEY_X: return vela::core::Key::X;
            case GLFW_KEY_Y: return vela::core::Key::Y;
            case GLFW_KEY_Z: return vela::core::Key::Z;

            case GLFW_KEY_0: return vela::core::Key::Num0;
            case GLFW_KEY_1: return vela::core::Key::Num1;
            case GLFW_KEY_2: return vela::core::Key::Num2;
            case GLFW_KEY_3: return vela::core::Key::Num3;
            case GLFW_KEY_4: return vela::core::Key::Num4;
            case GLFW_KEY_5: return vela::core::Key::Num5;
            case GLFW_KEY_6: return vela::core::Key::Num6;
            case GLFW_KEY_7: return vela::core::Key::Num7;
            case GLFW_KEY_8: return vela::core::Key::Num8;
            case GLFW_KEY_9: return vela::core::Key::Num9;

            case GLFW_KEY_ESCAPE: return vela::core::Key::Escape;
            case GLFW_KEY_ENTER: return vela::core::Key::Enter;
            case GLFW_KEY_TAB: return vela::core::Key::Tab;
            case GLFW_KEY_BACKSPACE: return vela::core::Key::Backspace;
            case GLFW_KEY_SPACE: return vela::core::Key::Space;

            case GLFW_KEY_LEFT: return vela::core::Key::Left;
            case GLFW_KEY_RIGHT: return vela::core::Key::Right;
            case GLFW_KEY_UP: return vela::core::Key::Up;
            case GLFW_KEY_DOWN: return vela::core::Key::Down;

            case GLFW_KEY_LEFT_SHIFT: return vela::core::Key::LeftShift;
            case GLFW_KEY_LEFT_CONTROL: return vela::core::Key::LeftControl;
            case GLFW_KEY_LEFT_ALT: return vela::core::Key::LeftAlt;

            case GLFW_KEY_RIGHT_SHIFT: return vela::core::Key::RightShift;
            case GLFW_KEY_RIGHT_CONTROL: return vela::core::Key::RightControl;
            case GLFW_KEY_RIGHT_ALT: return vela::core::Key::RightAlt;

            case GLFW_KEY_F1: return vela::core::Key::F1;
            case GLFW_KEY_F2: return vela::core::Key::F2;
            case GLFW_KEY_F3: return vela::core::Key::F3;
            case GLFW_KEY_F4: return vela::core::Key::F4;
            case GLFW_KEY_F5: return vela::core::Key::F5;
            case GLFW_KEY_F6: return vela::core::Key::F6;
            case GLFW_KEY_F7: return vela::core::Key::F7;
            case GLFW_KEY_F8: return vela::core::Key::F8;
            case GLFW_KEY_F9: return vela::core::Key::F9;
            case GLFW_KEY_F10: return vela::core::Key::F10;
            case GLFW_KEY_F11: return vela::core::Key::F11;
            case GLFW_KEY_F12: return vela::core::Key::F12;

            default:
                return vela::core::Key::Unknown;
        }
    }

    int toGLFWMouseButton(vela::core::MouseButton mouseButton)
    {
        switch(mouseButton)
        {
            case vela::core::MouseButton::Left: return GLFW_MOUSE_BUTTON_LEFT;
            case vela::core::MouseButton::Right: return GLFW_MOUSE_BUTTON_RIGHT;
            case vela::core::MouseButton::Middle: return GLFW_MOUSE_BUTTON_MIDDLE;
            case vela::core::MouseButton::Extra1: return 3;
            case vela::core::MouseButton::Extra2: return 4;
        }

        return -1;
    }

    vela::core::MouseButton fromGLFWMouseButton(int button)
    {
        switch (button)
        {
            case GLFW_MOUSE_BUTTON_LEFT: return vela::core::MouseButton::Left;
            case GLFW_MOUSE_BUTTON_RIGHT: return vela::core::MouseButton::Right;
            case GLFW_MOUSE_BUTTON_MIDDLE: return vela::core::MouseButton::Middle; 
            case GLFW_MOUSE_BUTTON_4: return vela::core::MouseButton::Extra1; 
            case GLFW_MOUSE_BUTTON_5: return vela::core::MouseButton::Extra2; 
            default: return vela::core::MouseButton::Unknown; 
        } 
    }

    int toGLFWCursorMode(vela::core::CursorMode cursorMode)
    {
        switch(cursorMode)
        {
            case vela::core::CursorMode::Normal: return GLFW_CURSOR_NORMAL;
            case vela::core::CursorMode::Hidden: return GLFW_CURSOR_HIDDEN;
            case vela::core::CursorMode::Locked: return GLFW_CURSOR_CAPTURED;
        }

        return -1;
    }
} //namespace

namespace vela::windowing
{
    DesktopWindowBackend::DesktopWindowBackend()
    {
        glfwSetErrorCallback(&DesktopWindowBackend::glfwErrorCallback);

        if (!glfwInit())
            throw std::runtime_error("Failed to initialize GLFW");
    }

    std::span<const core::Event> DesktopWindowBackend::events(void* nativeWindow) const
    {
        const auto& events = m_events.find(nativeWindow);

        if(events == m_events.end())
            return {};
        
        return events->second;
    }

    bool DesktopWindowBackend::isKeyDown(void* nativeWindow, core::Key key) const
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindow);
        int state = glfwGetKey(glfwWindow, toGLFWKey(key));
        return state == GLFW_PRESS;
    }

    bool DesktopWindowBackend::isMouseButtonDown(void* nativeWindow, core::MouseButton button) const
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindow);
        int state = glfwGetMouseButton(glfwWindow, toGLFWMouseButton(button));
        return state == GLFW_PRESS;
    }

    void DesktopWindowBackend::getCursorPosition(void* nativeWindow, double& x, double& y) const
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindow);
        glfwGetCursorPos(glfwWindow, &x, &y);
    }

    void DesktopWindowBackend::setCursorMode(void* nativeWindow, core::CursorMode mode)
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindow);
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, toGLFWCursorMode(mode));
    }

    void DesktopWindowBackend::getFramebufferSize(void* nativeWindow, int& width, int& height)
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindow);
        glfwGetFramebufferSize(glfwWindow, &width, &height);
    }

    void DesktopWindowBackend::getWindowSize(void* nativeWindow, int& width, int& height)
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindow);
        glfwGetWindowSize(glfwWindow, &width, &height);
    }

    void DesktopWindowBackend::setTitle(void* nativeWindow, const std::string& title)
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindow);
        glfwSetWindowTitle(glfwWindow, title.c_str());
    }

    void DesktopWindowBackend::glfwErrorCallback(int errorCode, const char* description)
    {
        std::cerr << "[DesktopWindowBackend] Code: " << errorCode
              << " Description: "
              << description
              << '\n';
    }

    void DesktopWindowBackend::pollEvents()
    {
        m_events.clear();
        glfwPollEvents();
    }

    void DesktopWindowBackend::waitEvents()
    {
        glfwWaitEvents();
    }

    std::vector<const char*> DesktopWindowBackend::requiredInstanceExtensions() const
    {
        uint32_t count = 0;
        const char** exts = glfwGetRequiredInstanceExtensions(&count);
        return std::vector<const char*>(exts, exts + count);
    }

    void* DesktopWindowBackend::createSurface(void* instance, void* nativeWindowHandle)
    {
        auto vkInstance = static_cast<VkInstance>(instance);
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindowHandle);
        VkSurfaceKHR surface;

        if(VkResult result = glfwCreateWindowSurface(vkInstance, glfwWindow, nullptr, &surface); result != VK_SUCCESS)
            throw std::runtime_error("Failed to create GLFW surface");

        return surface;
    }

    void DesktopWindowBackend::close(void* nativeWindow)
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindow);
        glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
    }

    std::vector<core::MonitorInfo> DesktopWindowBackend::monitors() const
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

    void* DesktopWindowBackend::createNativeWindow(const core::WindowPreferences& windowPreferences)
    {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, windowPreferences.resizable ? GLFW_TRUE : GLFW_FALSE);


        GLFWmonitor* monitor = monitorAt(windowPreferences.monitorIndex);

        const bool fullscreen = windowPreferences.mode != core::WindowMode::Windowed && monitor != nullptr;

        GLFWwindow* window = nullptr;

        if (!fullscreen)
        {
            window = glfwCreateWindow(windowPreferences.width, windowPreferences.height,
                windowPreferences.title.c_str(), nullptr, nullptr);
        }
        else if (windowPreferences.mode == core::WindowMode::BorderlessFullscreen)
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

        glfwSetWindowUserPointer(window, this);

        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
        {
            auto windowBackend = static_cast<DesktopWindowBackend*>(glfwGetWindowUserPointer(window));
            core::Event event;
            event.height = height;
            event.width = width;
            event.type = core::EventType::Resized;
            windowBackend->m_events[static_cast<void*>(window)].push_back(event);
        });

        glfwSetWindowFocusCallback(window, [](GLFWwindow* window, int focused)
        {
            auto windowBackend = static_cast<DesktopWindowBackend*>(glfwGetWindowUserPointer(window));
            core::Event event;
            event.type = focused ? core::EventType::FocusGained : core::EventType::FocusLost;

            windowBackend->m_events[static_cast<void*>(window)].push_back(event);
        });

        glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            auto windowBackend = static_cast<DesktopWindowBackend*>(glfwGetWindowUserPointer(window));
            core::Event event;

            event.modifiers.control = (mods & GLFW_MOD_CONTROL) != 0;
            event.modifiers.shift = (mods & GLFW_MOD_SHIFT) != 0;
            event.modifiers.alt = (mods & GLFW_MOD_ALT) != 0;

            event.key = fromGLFWKey(key);

            switch(action)
            {
                case GLFW_PRESS:
                    event.type = core::EventType::KeyPressed;
                    break;
                case GLFW_RELEASE:
                    event.type = core::EventType::KeyReleased;
                case GLFW_REPEAT:
                    event.type = core::EventType::KeyPressed;
                    break;
            }

            windowBackend->m_events[static_cast<void*>(window)].push_back(event);
        });

        glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods)
        {
            auto* windowBackend = static_cast<DesktopWindowBackend*>(glfwGetWindowUserPointer(window));

            core::Event event;

            switch(action)
            {
                case GLFW_PRESS:
                    event.type = core::EventType::MouseButtonPressed;
                    break;
                case GLFW_RELEASE:
                    event.type = core::EventType::MouseButtonReleased;
            }

            event.button = fromGLFWMouseButton(button);

            windowBackend->m_events[static_cast<void*>(window)].push_back(event);
        });

        glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos)
        {
            auto* windowBackend = static_cast<DesktopWindowBackend*>(glfwGetWindowUserPointer(window));

            core::Event event;
            event.type = core::EventType::MouseMoved;

            event.mouseX = static_cast<float>(xpos);
            event.mouseY = static_cast<float>(ypos);

            windowBackend->m_events[static_cast<void*>(window)].push_back(event);
        });

        glfwSetScrollCallback(window, [](GLFWwindow* window, double xOffset, double yOffset)
        {
            auto* windowBackend = static_cast<DesktopWindowBackend*>(glfwGetWindowUserPointer(window));

            core::Event event;
            event.type = core::EventType::MouseScrolled;

            event.scrollX = static_cast<float>(xOffset);
            event.scrollY = static_cast<float>(yOffset);

            windowBackend->m_events[static_cast<void*>(window)].push_back(event);
        });

        glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int codepoint)
        {
            if (codepoint < 128) 
            {
                auto windowBackend = static_cast<DesktopWindowBackend*>(glfwGetWindowUserPointer(window));
                core::Event event;
                event.type = core::EventType::TextEntered;
                event.ascii = static_cast<char>(codepoint);
                windowBackend->m_events[static_cast<void*>(window)].push_back(event);
            }
        });

        glfwSetWindowCloseCallback(window, [](GLFWwindow* window)
        {
            auto* windowBackend = static_cast<DesktopWindowBackend*>(glfwGetWindowUserPointer(window));

            core::Event event;
            event.type = core::EventType::WindowCloseRequested;

            windowBackend->m_events[static_cast<void*>(window)].push_back(event);
        });

        m_windowedRects[window] = rect;

        return window;
    }

    void DesktopWindowBackend::setMode(void* nativeWindow, core::WindowMode mode, uint32_t monitorIndex)
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

        if (mode == core::WindowMode::Windowed || monitor == nullptr)
        {
            glfwSetWindowMonitor(glfwWindow, nullptr, rect.x, rect.y, rect.width, rect.height, GLFW_DONT_CARE);
            return;
        }

        const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);

        if (mode == core::WindowMode::BorderlessFullscreen)
        {
            glfwSetWindowMonitor(glfwWindow, monitor, 0, 0,
                videoMode->width, videoMode->height, videoMode->refreshRate);
        }
        else
        {
            glfwSetWindowMonitor(glfwWindow, monitor, 0, 0, rect.width, rect.height, GLFW_DONT_CARE);
        }
    }

    void DesktopWindowBackend::destroyNativeWindow(void* window)
    {
        auto glfwWindow = static_cast<GLFWwindow*>(window);

        m_windowedRects.erase(window);
        m_events.erase(window);

        glfwDestroyWindow(glfwWindow);
    }

    bool DesktopWindowBackend::isOpen(void* window) const
    {
        auto glfwWindow = static_cast<GLFWwindow*>(window);

        return !glfwWindowShouldClose(glfwWindow);
    }

    DesktopWindowBackend::~DesktopWindowBackend()
    {
        glfwTerminate();
    }
} //namespace vela::windowing

namespace vela::core
{
    IWindowBackend& platformWindowBackend()
    {
        static windowing::DesktopWindowBackend backend;
        return backend;
    }
} //namespace vela::core