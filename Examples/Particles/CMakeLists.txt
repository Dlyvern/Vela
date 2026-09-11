add_executable(Particles Particles.cpp)
target_link_libraries(Particles PRIVATE Vela::Vela Vela::Assets)

vela_compile_shaders(
    TARGET Particles
    SOURCES
        ../shaders/particles_update.comp
        ../shaders/particles_draw.vert
        ../shaders/particles_draw.frag
)
