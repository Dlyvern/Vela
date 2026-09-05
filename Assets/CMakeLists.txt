cmake_minimum_required(VERSION 3.16)

add_library(VelaAssets STATIC
        src/Image.cpp
        src/Shader.cpp
        include/Vela/Assets/Image.hpp
        include/Vela/Assets/Shader.hpp
        include/Vela/Assets/Resources.hpp
)

target_include_directories(VelaAssets PUBLIC include)

target_link_libraries(VelaAssets
    PUBLIC
    VelaGraphics
    VelaFoundation
    PRIVATE
    stb_image
)

add_library(Vela::Assets ALIAS VelaAssets)
