#ifndef VELA_CORE_WINDOW_TYPES_HPP
#define VELA_CORE_WINDOW_TYPES_HPP

#include <cstdint>
#include <string>

namespace vela::core
{
    enum class WindowMode : uint8_t
    {
        eWINDOWED = 0,
        eBORDERLESS_FULLSCREEN,
        eEXCLUSIVE_FULLSCREEN
    };

    struct MonitorInfo
    {
        std::string name;
        int x{0};
        int y{0};
        int width{0};
        int height{0};
        int refreshRate{0};
    };

    struct WindowPreferences
    {
        std::string title{"Test"};
        int width{800};
        int height{600};
        WindowMode mode{WindowMode::eWINDOWED};
        uint32_t monitorIndex{0};
        bool resizable{true};
    };
} //namespace vela::core

#endif //VELA_CORE_WINDOW_TYPES_HPP
