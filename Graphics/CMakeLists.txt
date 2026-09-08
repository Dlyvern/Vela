add_library(VelaGraphics INTERFACE)
add_library(Vela::Graphics ALIAS VelaGraphics)

vela_public_includes(VelaGraphics INTERFACE)

target_link_libraries(VelaGraphics INTERFACE VelaMath VelaFoundation)

vela_install_target(VelaGraphics
    EXPORT_NAME Graphics
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include
)
