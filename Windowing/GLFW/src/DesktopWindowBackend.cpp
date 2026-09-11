#include "Vela/Windowing/DesktopWindowBackend.hpp"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>

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

    vela::core::GamepadButton fromGLFWGamepadButton(int button)
    {
        switch (button)
        {
            case GLFW_GAMEPAD_BUTTON_A:             return vela::core::GamepadButton::A;
            case GLFW_GAMEPAD_BUTTON_B:             return vela::core::GamepadButton::B;
            case GLFW_GAMEPAD_BUTTON_X:             return vela::core::GamepadButton::X;
            case GLFW_GAMEPAD_BUTTON_Y:             return vela::core::GamepadButton::Y;

            case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER:   return vela::core::GamepadButton::LEFT_BUMPER;
            case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER:  return vela::core::GamepadButton::RIGHT_BUMPER;

            case GLFW_GAMEPAD_BUTTON_BACK:          return vela::core::GamepadButton::BACK;
            case GLFW_GAMEPAD_BUTTON_START:         return vela::core::GamepadButton::START;
            case GLFW_GAMEPAD_BUTTON_GUIDE:         return vela::core::GamepadButton::GUIDE;

            case GLFW_GAMEPAD_BUTTON_LEFT_THUMB:    return vela::core::GamepadButton::LEFT_THUMB;
            case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB:   return vela::core::GamepadButton::RIGHT_THUMB;

            case GLFW_GAMEPAD_BUTTON_DPAD_UP:      return vela::core::GamepadButton::DPAD_UP;
            case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT:   return vela::core::GamepadButton::DPAD_RIGHT;
            case GLFW_GAMEPAD_BUTTON_DPAD_DOWN:    return vela::core::GamepadButton::DPAD_DOWN;
            case GLFW_GAMEPAD_BUTTON_DPAD_LEFT:    return vela::core::GamepadButton::DPAD_LEFT;

            default:                                return vela::core::GamepadButton::None;
        }
    }

    int toGLFWGamepadButton(vela::core::GamepadButton button)
    {
        switch (button)
        {
            case vela::core::GamepadButton::A: return GLFW_GAMEPAD_BUTTON_A;
            case vela::core::GamepadButton::B: return GLFW_GAMEPAD_BUTTON_B;
            case vela::core::GamepadButton::X: return GLFW_GAMEPAD_BUTTON_X;
            case vela::core::GamepadButton::Y: return GLFW_GAMEPAD_BUTTON_Y;
            case vela::core::GamepadButton::LEFT_BUMPER: return GLFW_GAMEPAD_BUTTON_LEFT_BUMPER;
            case vela::core::GamepadButton::RIGHT_BUMPER: return GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER;
            case vela::core::GamepadButton::BACK: return GLFW_GAMEPAD_BUTTON_BACK;
            case vela::core::GamepadButton::START: return GLFW_GAMEPAD_BUTTON_START;
            case vela::core::GamepadButton::GUIDE: return GLFW_GAMEPAD_BUTTON_GUIDE;
            case vela::core::GamepadButton::LEFT_THUMB: return GLFW_GAMEPAD_BUTTON_LEFT_THUMB;
            case vela::core::GamepadButton::RIGHT_THUMB: return GLFW_GAMEPAD_BUTTON_RIGHT_THUMB;
            case vela::core::GamepadButton::DPAD_UP: return GLFW_GAMEPAD_BUTTON_DPAD_UP;
            case vela::core::GamepadButton::DPAD_RIGHT: return GLFW_GAMEPAD_BUTTON_DPAD_RIGHT;
            case vela::core::GamepadButton::DPAD_DOWN: return GLFW_GAMEPAD_BUTTON_DPAD_DOWN;
            case vela::core::GamepadButton::DPAD_LEFT: return GLFW_GAMEPAD_BUTTON_DPAD_LEFT;
            case vela::core::GamepadButton::LAST: return -1;
        }

        return -1;
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
                
            case vela::core::Key::Num0: return GLFW_KEY_0;
            case vela::core::Key::Num1: return GLFW_KEY_1;
            case vela::core::Key::Num2: return GLFW_KEY_2;
            case vela::core::Key::Num3: return GLFW_KEY_3;
            case vela::core::Key::Num4: return GLFW_KEY_4;
            case vela::core::Key::Num5: return GLFW_KEY_5;
            case vela::core::Key::Num6: return GLFW_KEY_6;
            case vela::core::Key::Num7: return GLFW_KEY_7;
            case vela::core::Key::Num8: return GLFW_KEY_8;
            case vela::core::Key::Num9: return GLFW_KEY_9;

            case vela::core::Key::Numpad0: return GLFW_KEY_KP_0;
            case vela::core::Key::Numpad1: return GLFW_KEY_KP_1;
            case vela::core::Key::Numpad2: return GLFW_KEY_KP_2;
            case vela::core::Key::Numpad3: return GLFW_KEY_KP_3;
            case vela::core::Key::Numpad4: return GLFW_KEY_KP_4;
            case vela::core::Key::Numpad5: return GLFW_KEY_KP_5;
            case vela::core::Key::Numpad6: return GLFW_KEY_KP_6;
            case vela::core::Key::Numpad7: return GLFW_KEY_KP_7;
            case vela::core::Key::Numpad8: return GLFW_KEY_KP_8;
            case vela::core::Key::Numpad9: return GLFW_KEY_KP_9;
            case vela::core::Key::NumpadDecimal: return GLFW_KEY_KP_DECIMAL;
            case vela::core::Key::NumpadDivide: return GLFW_KEY_KP_DIVIDE;
            case vela::core::Key::NumpadMultiply: return GLFW_KEY_KP_MULTIPLY;
            case vela::core::Key::NumpadSubtract: return GLFW_KEY_KP_SUBTRACT;
            case vela::core::Key::NumpadAdd: return GLFW_KEY_KP_ADD;
            case vela::core::Key::NumpadEnter: return GLFW_KEY_KP_ENTER;
            case vela::core::Key::NumpadEqual: return GLFW_KEY_KP_EQUAL;

            case vela::core::Key::Escape: return GLFW_KEY_ESCAPE;
            case vela::core::Key::Enter: return GLFW_KEY_ENTER;
            case vela::core::Key::Tab: return GLFW_KEY_TAB;
            case vela::core::Key::Backspace: return GLFW_KEY_BACKSPACE;
            case vela::core::Key::Space: return GLFW_KEY_SPACE;
            case vela::core::Key::Insert: return GLFW_KEY_INSERT;
            case vela::core::Key::Delete: return GLFW_KEY_DELETE;

            case vela::core::Key::Left: return GLFW_KEY_LEFT;
            case vela::core::Key::Right: return GLFW_KEY_RIGHT;
            case vela::core::Key::Up: return GLFW_KEY_UP;
            case vela::core::Key::Down: return GLFW_KEY_DOWN;
            case vela::core::Key::PageUp: return GLFW_KEY_PAGE_UP;
            case vela::core::Key::PageDown: return GLFW_KEY_PAGE_DOWN;
            case vela::core::Key::Home: return GLFW_KEY_HOME;
            case vela::core::Key::End: return GLFW_KEY_END;

            case vela::core::Key::CapsLock: return GLFW_KEY_CAPS_LOCK;
            case vela::core::Key::ScrollLock: return GLFW_KEY_SCROLL_LOCK;
            case vela::core::Key::NumLock: return GLFW_KEY_NUM_LOCK;
            case vela::core::Key::PrintScreen: return GLFW_KEY_PRINT_SCREEN;
            case vela::core::Key::Pause: return GLFW_KEY_PAUSE;

            case vela::core::Key::LeftShift: return GLFW_KEY_LEFT_SHIFT;
            case vela::core::Key::LeftControl: return GLFW_KEY_LEFT_CONTROL;
            case vela::core::Key::LeftAlt: return GLFW_KEY_LEFT_ALT;
            case vela::core::Key::LeftSuper: return GLFW_KEY_LEFT_SUPER;

            case vela::core::Key::RightShift: return GLFW_KEY_RIGHT_SHIFT;
            case vela::core::Key::RightControl: return GLFW_KEY_RIGHT_CONTROL;
            case vela::core::Key::RightAlt: return GLFW_KEY_RIGHT_ALT;
            case vela::core::Key::RightSuper: return GLFW_KEY_RIGHT_SUPER;
            case vela::core::Key::Menu: return GLFW_KEY_MENU;

            case vela::core::Key::Apostrophe: return GLFW_KEY_APOSTROPHE;
            case vela::core::Key::Comma: return GLFW_KEY_COMMA;
            case vela::core::Key::Minus: return GLFW_KEY_MINUS;
            case vela::core::Key::Period: return GLFW_KEY_PERIOD;
            case vela::core::Key::Slash: return GLFW_KEY_SLASH;
            case vela::core::Key::Semicolon: return GLFW_KEY_SEMICOLON;
            case vela::core::Key::Equal: return GLFW_KEY_EQUAL;
            case vela::core::Key::LeftBracket: return GLFW_KEY_LEFT_BRACKET;
            case vela::core::Key::Backslash: return GLFW_KEY_BACKSLASH;
            case vela::core::Key::RightBracket: return GLFW_KEY_RIGHT_BRACKET;
            case vela::core::Key::GraveAccent: return GLFW_KEY_GRAVE_ACCENT;

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
            case vela::core::Key::F13: return GLFW_KEY_F13;
            case vela::core::Key::F14: return GLFW_KEY_F14;
            case vela::core::Key::F15: return GLFW_KEY_F15;
            case vela::core::Key::F16: return GLFW_KEY_F16;
            case vela::core::Key::F17: return GLFW_KEY_F17;
            case vela::core::Key::F18: return GLFW_KEY_F18;
            case vela::core::Key::F19: return GLFW_KEY_F19;
            case vela::core::Key::F20: return GLFW_KEY_F20;
            case vela::core::Key::F21: return GLFW_KEY_F21;
            case vela::core::Key::F22: return GLFW_KEY_F22;
            case vela::core::Key::F23: return GLFW_KEY_F23;
            case vela::core::Key::F24: return GLFW_KEY_F24;
            case vela::core::Key::F25: return GLFW_KEY_F25;
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

            case GLFW_KEY_KP_0: return vela::core::Key::Numpad0;
            case GLFW_KEY_KP_1: return vela::core::Key::Numpad1;
            case GLFW_KEY_KP_2: return vela::core::Key::Numpad2;
            case GLFW_KEY_KP_3: return vela::core::Key::Numpad3;
            case GLFW_KEY_KP_4: return vela::core::Key::Numpad4;
            case GLFW_KEY_KP_5: return vela::core::Key::Numpad5;
            case GLFW_KEY_KP_6: return vela::core::Key::Numpad6;
            case GLFW_KEY_KP_7: return vela::core::Key::Numpad7;
            case GLFW_KEY_KP_8: return vela::core::Key::Numpad8;
            case GLFW_KEY_KP_9: return vela::core::Key::Numpad9;
            case GLFW_KEY_KP_DECIMAL: return vela::core::Key::NumpadDecimal;
            case GLFW_KEY_KP_DIVIDE: return vela::core::Key::NumpadDivide;
            case GLFW_KEY_KP_MULTIPLY: return vela::core::Key::NumpadMultiply;
            case GLFW_KEY_KP_SUBTRACT: return vela::core::Key::NumpadSubtract;
            case GLFW_KEY_KP_ADD: return vela::core::Key::NumpadAdd;
            case GLFW_KEY_KP_ENTER: return vela::core::Key::NumpadEnter;
            case GLFW_KEY_KP_EQUAL: return vela::core::Key::NumpadEqual;

            case GLFW_KEY_ESCAPE: return vela::core::Key::Escape;
            case GLFW_KEY_ENTER: return vela::core::Key::Enter;
            case GLFW_KEY_TAB: return vela::core::Key::Tab;
            case GLFW_KEY_BACKSPACE: return vela::core::Key::Backspace;
            case GLFW_KEY_SPACE: return vela::core::Key::Space;
            case GLFW_KEY_INSERT: return vela::core::Key::Insert;
            case GLFW_KEY_DELETE: return vela::core::Key::Delete;

            case GLFW_KEY_LEFT: return vela::core::Key::Left;
            case GLFW_KEY_RIGHT: return vela::core::Key::Right;
            case GLFW_KEY_UP: return vela::core::Key::Up;
            case GLFW_KEY_DOWN: return vela::core::Key::Down;
            case GLFW_KEY_PAGE_UP: return vela::core::Key::PageUp;
            case GLFW_KEY_PAGE_DOWN: return vela::core::Key::PageDown;
            case GLFW_KEY_HOME: return vela::core::Key::Home;
            case GLFW_KEY_END: return vela::core::Key::End;

            case GLFW_KEY_CAPS_LOCK: return vela::core::Key::CapsLock;
            case GLFW_KEY_SCROLL_LOCK: return vela::core::Key::ScrollLock;
            case GLFW_KEY_NUM_LOCK: return vela::core::Key::NumLock;
            case GLFW_KEY_PRINT_SCREEN: return vela::core::Key::PrintScreen;
            case GLFW_KEY_PAUSE: return vela::core::Key::Pause;

            case GLFW_KEY_LEFT_SHIFT: return vela::core::Key::LeftShift;
            case GLFW_KEY_LEFT_CONTROL: return vela::core::Key::LeftControl;
            case GLFW_KEY_LEFT_ALT: return vela::core::Key::LeftAlt;
            case GLFW_KEY_LEFT_SUPER: return vela::core::Key::LeftSuper;

            case GLFW_KEY_RIGHT_SHIFT: return vela::core::Key::RightShift;
            case GLFW_KEY_RIGHT_CONTROL: return vela::core::Key::RightControl;
            case GLFW_KEY_RIGHT_ALT: return vela::core::Key::RightAlt;
            case GLFW_KEY_RIGHT_SUPER: return vela::core::Key::RightSuper;
            case GLFW_KEY_MENU: return vela::core::Key::Menu;

            case GLFW_KEY_APOSTROPHE: return vela::core::Key::Apostrophe;
            case GLFW_KEY_COMMA: return vela::core::Key::Comma;
            case GLFW_KEY_MINUS: return vela::core::Key::Minus;
            case GLFW_KEY_PERIOD: return vela::core::Key::Period;
            case GLFW_KEY_SLASH: return vela::core::Key::Slash;
            case GLFW_KEY_SEMICOLON: return vela::core::Key::Semicolon;
            case GLFW_KEY_EQUAL: return vela::core::Key::Equal;
            case GLFW_KEY_LEFT_BRACKET: return vela::core::Key::LeftBracket;
            case GLFW_KEY_BACKSLASH: return vela::core::Key::Backslash;
            case GLFW_KEY_RIGHT_BRACKET: return vela::core::Key::RightBracket;
            case GLFW_KEY_GRAVE_ACCENT: return vela::core::Key::GraveAccent;

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
            case GLFW_KEY_F13: return vela::core::Key::F13;
            case GLFW_KEY_F14: return vela::core::Key::F14;
            case GLFW_KEY_F15: return vela::core::Key::F15;
            case GLFW_KEY_F16: return vela::core::Key::F16;
            case GLFW_KEY_F17: return vela::core::Key::F17;
            case GLFW_KEY_F18: return vela::core::Key::F18;
            case GLFW_KEY_F19: return vela::core::Key::F19;
            case GLFW_KEY_F20: return vela::core::Key::F20;
            case GLFW_KEY_F21: return vela::core::Key::F21;
            case GLFW_KEY_F22: return vela::core::Key::F22;
            case GLFW_KEY_F23: return vela::core::Key::F23;
            case GLFW_KEY_F24: return vela::core::Key::F24;
            case GLFW_KEY_F25: return vela::core::Key::F25;

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
            case vela::core::MouseButton::Unknown: return GLFW_KEY_UNKNOWN;
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

    int toGLFWCursorShape(vela::core::CursorShape cursorShape)
    {
        switch (cursorShape)
        {
            case vela::core::CursorShape::Arrow:      return GLFW_ARROW_CURSOR;
            case vela::core::CursorShape::TextInput:  return GLFW_IBEAM_CURSOR;
            case vela::core::CursorShape::Hand:       return GLFW_POINTING_HAND_CURSOR;
            case vela::core::CursorShape::NotAllowed: return GLFW_NOT_ALLOWED_CURSOR;
            case vela::core::CursorShape::ResizeAll:  return GLFW_RESIZE_ALL_CURSOR;
            case vela::core::CursorShape::ResizeNS:   return GLFW_RESIZE_NS_CURSOR;
            case vela::core::CursorShape::ResizeEW:   return GLFW_RESIZE_EW_CURSOR;
            case vela::core::CursorShape::ResizeNESW: return GLFW_RESIZE_NESW_CURSOR;
            case vela::core::CursorShape::ResizeNWSE: return GLFW_RESIZE_NWSE_CURSOR;
        }

        return GLFW_ARROW_CURSOR;
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

    void DesktopWindowBackend::setCursorShape(void* nativeWindow, core::CursorShape shape)
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindow);

        auto cursorIt = m_cursors.find(shape);

        if (cursorIt == m_cursors.end())
            cursorIt = m_cursors.emplace(shape, glfwCreateStandardCursor(toGLFWCursorShape(shape))).first;

        if (cursorIt->second == nullptr)
        {
            glfwSetCursor(glfwWindow, nullptr);
            return;
        }

        glfwSetCursor(glfwWindow, cursorIt->second);
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

    bool DesktopWindowBackend::popEvent(void* nativeWindowHandle, core::Event& event)
    {
        const auto queue = m_events.find(nativeWindowHandle);

        if (queue == m_events.end() || queue->second.empty())
            return false;

        event = queue->second.front();
        queue->second.erase(queue->second.begin());

        return true;
    }

    void DesktopWindowBackend::pollEvents(void* nativeWindowHandle)
    {
        if (const auto queue = m_events.find(nativeWindowHandle); queue != m_events.end())
            queue->second.clear();

        glfwPollEvents();
        pollGamepadEvents(nativeWindowHandle);
    }

    void DesktopWindowBackend::pollGamepadEvents(void* nativeWindowHandle)
    {
        auto glfwWindow = static_cast<GLFWwindow*>(nativeWindowHandle);

        if(!glfwWindow)
            return;

        std::vector<int> disconnected;

        for(auto& gamepad : m_gamepads)
        {
            GLFWgamepadstate state;

            if(!glfwGetGamepadState(gamepad.id, &state) || !gamepad.isConnected)
            {
                disconnected.push_back(gamepad.id);

                core::Event disconnectEvent{};
                disconnectEvent.gamepadId = gamepad.id;
                disconnectEvent.type = core::EventType::GamepadDisconnected;
                m_events[nativeWindowHandle].push_back(disconnectEvent);

                std::cerr << "Joystick " << gamepad.id << " disconnected\n";

                continue;
            }

            if(gamepad.isConnected && !gamepad.firedAboutConnection)
            {
                core::Event connectedEvent{};
                connectedEvent.gamepadId = gamepad.id;
                connectedEvent.type = core::EventType::GamepadConnected;
                m_events[nativeWindowHandle].push_back(connectedEvent);

                std::cerr << "Joystick " << gamepad.id << " connected\n";

                gamepad.firedAboutConnection = true;

                updateGamepadState(gamepad);
            }
            
            bool axisMoved{false};

            if(getGamepadAxisLeftX(gamepad.id) != gamepad.prevGamepadLeftAxisX)
                axisMoved = true;
            if(getGamepadAxisLeftY(gamepad.id) != gamepad.prevGamepadLeftAxisY)
                axisMoved = true;
            if(getGamepadAxisRightX(gamepad.id) != gamepad.prevGamepadRightAxisX)
                axisMoved = true;
            if(getGamepadAxisRightY(gamepad.id) != gamepad.prevGamepadRightAxisY)
                axisMoved = true;
            
            if(axisMoved)
            {
                core::Event moveAxisEvent{};
                moveAxisEvent.gamepadId = gamepad.id;
                moveAxisEvent.type = core::EventType::GamepadAxisMoved;
                m_events[nativeWindowHandle].push_back(moveAxisEvent);
            }

            for(int glfwButton = 0; glfwButton <= GLFW_GAMEPAD_BUTTON_LAST; ++glfwButton)
            {
                const core::GamepadButton button = fromGLFWGamepadButton(glfwButton);

                if(button == core::GamepadButton::None || button == core::GamepadButton::LAST)
                    continue;

                const size_t buttonIndex = static_cast<size_t>(button);
                const bool isDown = state.buttons[glfwButton] == GLFW_PRESS;

                if(isDown == gamepad.prevButtons[buttonIndex])
                    continue;

                core::Event buttonEvent{};
                buttonEvent.gamepadId = gamepad.id;
                buttonEvent.gamepadButton = button;
                buttonEvent.type = isDown ? core::EventType::GamepadButtonPressed
                                          : core::EventType::GamepadButtonReleased;

                m_events[nativeWindowHandle].push_back(buttonEvent);

                gamepad.prevButtons[buttonIndex] = isDown;
            }

            updateGamepadState(gamepad);
        }

        for(int id : disconnected)
            m_gamepads.erase(std::remove_if(m_gamepads.begin(), m_gamepads.end(),
                [id](const GamepadState& state) { return state.id == id; }), m_gamepads.end());
    }

    void DesktopWindowBackend::updateGamepadState(GamepadState& gamepadState)
    {
        gamepadState.prevGamepadLeftAxisX = getGamepadAxisLeftX(gamepadState.id);
        gamepadState.prevGamepadLeftAxisY = getGamepadAxisLeftY(gamepadState.id);
        gamepadState.prevGamepadRightAxisX = getGamepadAxisRightX(gamepadState.id);
        gamepadState.prevGamepadRightAxisY = getGamepadAxisRightY(gamepadState.id);
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
            event.modifiers.super = (mods & GLFW_MOD_SUPER) != 0;
            event.modifiers.capsLock = (mods & GLFW_MOD_CAPS_LOCK) != 0;
            event.modifiers.numLock = (mods & GLFW_MOD_NUM_LOCK) != 0;

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
            auto windowBackend = static_cast<DesktopWindowBackend*>(glfwGetWindowUserPointer(window));

            core::Event event{};
            event.type = core::EventType::TextEntered;
            event.codepoint = codepoint;

            windowBackend->m_events[static_cast<void*>(window)].push_back(event);
        });

        glfwSetWindowCloseCallback(window, [](GLFWwindow* window)
        {
            auto* windowBackend = static_cast<DesktopWindowBackend*>(glfwGetWindowUserPointer(window));

            core::Event event;
            event.type = core::EventType::CloseRequested;

            windowBackend->m_events[static_cast<void*>(window)].push_back(event);
        });
        
        //Gamepad
        glfwSetJoystickCallback([](int jid, int event)
        {
            if(event == GLFW_CONNECTED)
            {
                DesktopWindowBackend::m_gamepads.push_back({true, jid});
            }

            else if(event == GLFW_DISCONNECTED)
            {
                auto it = std::find_if(m_gamepads.begin(), m_gamepads.end(),
                    [jid](const GamepadState& state) { return state.id == jid; });

                if(it != m_gamepads.end())
                    it->isConnected = false;
            }
        });

        m_windowedRects[window] = rect;

        //* If gamepad was connected before executing programm, glfw won't tell us it using callback so iterate after creating window throught all connected gamepads
        for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; jid++)
        {
            if (glfwJoystickPresent(jid) && glfwJoystickIsGamepad(jid))
            {
                auto it = std::find_if(m_gamepads.begin(), m_gamepads.end(), [jid](GamepadState state) { return state.id == jid; });

                if(it != m_gamepads.end())
                    continue;

                m_gamepads.push_back({true, jid});
            }
        }

        if(!m_gamepads.empty())
            std::cout << "Found " << m_gamepads.size() << " connected gamepads\n";

        return window;
    }

    std::string DesktopWindowBackend::getGamepadName(int jid) const
    {
        const char* name = glfwGetGamepadName(jid);

        if(!name)
            return {};

        return name; 
    }

    bool DesktopWindowBackend::isGamepadConnected(int jid) const
    {
        auto it = std::find_if(m_gamepads.begin(), m_gamepads.end(), [jid](GamepadState state) { return state.id == jid; });
        return it != m_gamepads.end();
    }

    bool DesktopWindowBackend::isGamepadButtonDown(int jid, core::GamepadButton button) const
    {
        GLFWgamepadstate state;

        if(!glfwGetGamepadState(jid, &state))
            return false;

        return state.buttons[toGLFWGamepadButton(button)] == GLFW_PRESS;
    }

    float DesktopWindowBackend::getGamepadAxisLeftX(int jid) const
    {
        GLFWgamepadstate state;

        if(!glfwGetGamepadState(jid, &state))
            return 0.0f;

        return state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
    }

    float DesktopWindowBackend::getGamepadAxisLeftY(int jid) const
    {
        GLFWgamepadstate state;

        if(!glfwGetGamepadState(jid, &state))
            return 0.0f;

        return state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
    }

    float DesktopWindowBackend::getGamepadAxisRightX(int jid) const
    {
        GLFWgamepadstate state;

        if(!glfwGetGamepadState(jid, &state))
            return 0.0f;

        return state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
    }

    float DesktopWindowBackend::getGamepadAxisRightY(int jid) const
    {
        GLFWgamepadstate state;

        if(!glfwGetGamepadState(jid, &state))
            return 0.0f;

        return state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
    }

    std::vector<int> DesktopWindowBackend::getConnectedGamepads() const
    {
        std::vector<int> connectedGamepads;

        for(const auto& gamepad : m_gamepads)
            connectedGamepads.push_back(gamepad.id);

        return connectedGamepads;
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
        for (const auto& [shape, cursor] : m_cursors)
            if (cursor != nullptr)
                glfwDestroyCursor(cursor);

        m_cursors.clear();

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