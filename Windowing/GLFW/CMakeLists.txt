
add_library(VelaGLFW STATIC include/Vela/Windowing/DesktopWindowBackend.hpp src/DesktopWindowBackend.cpp)
target_include_directories(VelaGLFW PUBLIC include) 
target_link_libraries(VelaGLFW PUBLIC VelaCore PRIVATE glfw volk_headers)
add_library(Vela::GLFW ALIAS VelaGLFW)
