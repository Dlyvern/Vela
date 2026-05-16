add_executable(SimpleCube SimpleCube.cpp)
target_link_libraries(SimpleCube PRIVATE Vela::Vela Vela::GLFW)

vela_compile_shaders(
    TARGET SimpleCube
    SOURCES
        ../../Shaders/triangle.vert
        ../../Shaders/triangle.frag
)
