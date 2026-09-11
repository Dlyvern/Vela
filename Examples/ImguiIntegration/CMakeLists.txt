add_executable(ImguiIntegration ImguiIntegration.cpp)
target_link_libraries(ImguiIntegration PRIVATE Vela::Vela Vela::Assets Vela::Builtins imgui)

vela_compile_shaders(
    TARGET ImguiIntegration
    SOURCES
        ../shaders/imgui_shader.vert
        ../shaders/imgui_shader.frag
)
