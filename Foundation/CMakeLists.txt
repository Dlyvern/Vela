add_library(VelaFoundation INTERFACE)
add_library(Vela::Foundation ALIAS VelaFoundation)

vela_public_includes(VelaFoundation INTERFACE)

vela_install_target(VelaFoundation
    EXPORT_NAME Foundation
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include
)
