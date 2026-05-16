#ifndef VELA_CORE_WINDOW_HPP
#define VELA_CORE_WINDOW_HPP

#include <memory>

#include "Vela/Core/IWindowBackend.hpp"

namespace vela::core
{
    class Window
    {
    public:
        Window(IWindowBackend& windowBackend, int w, int h, const std::string& title);
        bool isOpen() const;
        IWindowBackend& getWindowBackend();
        void* const getNativeHandle();
        void pollEvents();
        ~Window();
    
    private:
        IWindowBackend& m_windowBackend;
        void* m_nativeWindow{nullptr};
    };
} //namespace vela::core

#endif //VELA_CORE_WINDOW_HPP