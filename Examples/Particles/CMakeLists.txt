add_executable(Particles Particles.cpp)
target_link_libraries(Particles PRIVATE Vela::Vela Vela::Assets Vela::Builtins)

# add_custom_command(TARGET Particles POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#         ${CMAKE_CURRENT_SOURCE_DIR}/Cat.png
#         $<TARGET_FILE_DIR:Particles>/Cat.png
#     COMMENT "Staging assets for Particles"
# )

# vela_compile_shaders(
#     TARGET Particles
#     SOURCES
#         ../../Shaders/sprite_shader.vert
#         ../../Shaders/sprite_shader.frag
#         ../../Shaders/static_shader.vert
#         ../../Shaders/static_shader.frag
# )
