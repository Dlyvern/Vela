#ifndef VELA_CORE_INPUT_HPP
#define VELA_CORE_INPUT_HPP

#include <cstdint>

namespace vela::core
{
    enum class Key : uint16_t
    {
        Unknown = 0,

        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

        Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
        Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
        NumpadDecimal, NumpadDivide, NumpadMultiply,
        NumpadSubtract, NumpadAdd, NumpadEnter, NumpadEqual,

        Escape, Enter, Tab, Backspace, Space, Insert, Delete,

        Left, Right, Up, Down,
        PageUp, PageDown, Home, End,

        CapsLock, ScrollLock, NumLock, PrintScreen, Pause,

        LeftShift, LeftControl, LeftAlt, LeftSuper,
        RightShift, RightControl, RightAlt, RightSuper,
        Menu,

        Apostrophe, Comma, Minus, Period, Slash, Semicolon, Equal,
        LeftBracket, Backslash, RightBracket, GraveAccent,

        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25
    };

    enum class GamepadButton : uint8_t
    {
        None = 0,
        A,
        B,
        X,
        Y,
        LEFT_BUMPER,
        RIGHT_BUMPER,
        BACK,
        START,
        GUIDE,
        LEFT_THUMB,
        RIGHT_THUMB,
        DPAD_UP,
        DPAD_RIGHT,
        DPAD_DOWN,
        DPAD_LEFT,
        LAST,
    };

    enum class MouseButton : uint8_t
    {
        Unknown = 0,
        Left,
        Right,
        Middle,
        Extra1,
        Extra2
    };

    enum class CursorMode : uint8_t 
    {
        Normal = 0,
        Hidden,
        Locked
    };

    struct KeyModifiers
    {
        bool shift{false};
        bool control{false};
        bool alt{false};
    };

    enum class EventType : uint8_t
    {
        None = 0,

        // Window

        /*
            Fired when the user requests to close the window (e.g. by pressing the close button).
            This event is not fired when the window is closed programmatically via Window::close()
        */
        CloseRequested,
        Resized,

        FocusGained, FocusLost,

        // Keyboard
        KeyPressed, 
        KeyReleased,
        TextEntered,

        // Mouse
        MouseButtonPressed,
        MouseButtonReleased,
        MouseMoved,
        MouseScrolled,

        // Gamepad
        GamepadConnected,
        GamepadDisconnected,
        GamepadButtonPressed,
        GamepadButtonReleased,
        GamepadAxisMoved
    };

    struct Event
    {
        EventType type{EventType::None};

        Key key{Key::Unknown};
        KeyModifiers modifiers{};
        MouseButton button{MouseButton::Left};

        float mouseX{0.0f};
        float mouseY{0.0f};
        float scrollX{0.0f};
        float scrollY{0.0f};

        float gamepadRightAxisX{0.0f};
        float gamepadRightAxisY{0.0f};
        float gamepadLeftAxisX{0.0f};
        float gamepadLeftAxisY{0.0f};

        GamepadButton gamepadButton{GamepadButton::None};

        uint32_t codepoint{0};

        int gamepadId{-1};

        uint32_t width{0};
        uint32_t height{0};
    };

} //namespace vela::core

#endif //VELA_CORE_INPUT_HPP