#ifndef VELA_CORE_WINDOW_HPP
#define VELA_CORE_WINDOW_HPP

#include "Vela/Core/IWindowBackend.hpp"
#include "Vela/Core/WindowTypes.hpp"

#include <cstdint>
#include <vector>

namespace vela::core
{
    class Window
    {
    public:
        Window(IWindowBackend& windowBackend, const WindowPreferences& windowPreferences);
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) noexcept;
        Window& operator=(Window&&) noexcept;

        bool isOpen() const;
        void close();

        IWindowBackend& getWindowBackend();
        void* getNativeHandle() const;

        void pollEvents();
        void setTitle(const std::string& title);

        void getSize(int& width, int& height) const;
        void getFramebufferSize(int& width, int& height) const;

        void setMode(WindowMode mode, uint32_t monitorIndex = 0);
        WindowMode getMode() const;

        std::vector<MonitorInfo> monitors() const;

        ~Window();

    private:
        void destroy();

        IWindowBackend* m_windowBackend{nullptr};
        void* m_nativeWindow{nullptr};
        WindowMode m_mode{WindowMode::eWINDOWED};
    };
} //namespace vela::core

#endif //VELA_CORE_WINDOW_HPP
