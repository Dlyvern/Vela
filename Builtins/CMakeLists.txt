cmake_minimum_required(VERSION 3.16)

add_library(VelaBuiltins STATIC
        src/ScenePass.cpp
        src/ForwardGraph.cpp
        src/Shapes.cpp
        include/Vela/Builtins/ScenePass.hpp
        include/Vela/Builtins/ForwardGraph.hpp
        include/Vela/Builtins/Shapes.hpp
)

target_include_directories(VelaBuiltins PUBLIC include)

target_link_libraries(VelaBuiltins
    PUBLIC
    VelaGraphics
    VelaFoundation
)

add_library(Vela::Builtins ALIAS VelaBuiltins)
