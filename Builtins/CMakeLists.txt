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

target_link_libraries(VelaBuiltins
    PUBLIC
    VelaGraphics
    VelaFoundation
)

vela_install_target(VelaBuiltins
    EXPORT_NAME Builtins
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include
)
