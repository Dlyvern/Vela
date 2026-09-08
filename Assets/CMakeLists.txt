add_library(VelaAssets STATIC
        src/Image.cpp
        src/Shader.cpp
        include/Vela/Assets/Image.hpp
        include/Vela/Assets/Shader.hpp
        include/Vela/Assets/Resources.hpp
)

add_library(Vela::Assets ALIAS VelaAssets)

vela_public_includes(VelaAssets PUBLIC)

target_link_libraries(VelaAssets
    PUBLIC
    VelaGraphics
    VelaFoundation
    PRIVATE
    stb_image
)

vela_install_target(VelaAssets
    EXPORT_NAME Assets
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include
)
