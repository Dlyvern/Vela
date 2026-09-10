add_executable(SimpleCube SimpleCube.cpp)
target_link_libraries(SimpleCube PRIVATE Vela::Vela Vela::Assets Vela::Builtins)

add_custom_command(TARGET SimpleCube POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Cat.png
        $<TARGET_FILE_DIR:SimpleCube>/Cat.png
    COMMENT "Staging assets for SimpleCube"
)

vela_compile_shaders(
    TARGET SimpleCube
    SOURCES
        ../../Shaders/sprite_shader.vert
        ../../Shaders/sprite_shader.frag
        ../../Shaders/static_shader.vert
        ../../Shaders/static_shader.frag
)
