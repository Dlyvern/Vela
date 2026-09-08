add_library(VelaMath INTERFACE)
add_library(Vela::Math ALIAS VelaMath)

vela_public_includes(VelaMath INTERFACE)

vela_install_target(VelaMath
    EXPORT_NAME Math
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include
)
