add_library(VelaBuiltins STATIC
        src/ScenePass.cpp
        src/ForwardGraph.cpp
        src/Shapes.cpp
        include/Vela/Builtins/ScenePass.hpp
        include/Vela/Builtins/ForwardGraph.hpp
        include/Vela/Builtins/Shapes.hpp
)

add_library(Vela::Builtins ALIAS VelaBuiltins)

vela_public_includes(VelaBuiltins PUBLIC)

# VelaBackend implements the Graphics API the built-ins call, so it must follow VelaBuiltins on the link line
# whatever order the application lists Vela's targets in
target_link_libraries(VelaBuiltins
    PUBLIC
    VelaGraphics
    VelaFoundation
    PRIVATE
    VelaBackend
)

vela_install_target(VelaBuiltins
    EXPORT_NAME Builtins
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include
)
