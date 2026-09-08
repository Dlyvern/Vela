add_library(VelaGLFW STATIC include/Vela/Windowing/DesktopWindowBackend.hpp src/DesktopWindowBackend.cpp)
add_library(Vela::GLFW ALIAS VelaGLFW)

vela_public_includes(VelaGLFW PUBLIC)

target_link_libraries(VelaGLFW PUBLIC VelaCore PRIVATE glfw volk_headers)

vela_install_target(VelaGLFW
    EXPORT_NAME GLFW
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include
)
