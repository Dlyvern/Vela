add_executable(LittleEngine
    src/main.cpp
    src/Model.cpp
    src/ImGuiLayer.cpp
    src/FlyCamera.cpp
    src/Camera.cpp
)

target_include_directories(LittleEngine PRIVATE include)

target_link_libraries(LittleEngine PRIVATE
    Vela::Vela
    Vela::Assets
    Vela::Builtins
    imgui
    cgltf
)

vela_compile_shaders(
    TARGET LittleEngine
    SOURCES
        shaders/lit.vert
        shaders/lit.frag
        shaders/imgui.vert
        shaders/imgui.frag
)
