add_executable(SimpleCube SimpleCube.cpp)
target_link_libraries(SimpleCube PRIVATE Vela::Vela Vela::GLFW)

vela_compile_shaders(
    TARGET SimpleCube
    SOURCES
        ../../Shaders/sprite_shader.vert
        ../../Shaders/sprite_shader.frag
        ../../Shaders/static_shader.vert
        ../../Shaders/static_shader.frag
)
