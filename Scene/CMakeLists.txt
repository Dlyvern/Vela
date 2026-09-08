set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)

add_library(VelaScene STATIC ${PROJECT_SOURCES})
add_library(Vela::Scene ALIAS VelaScene)

vela_public_includes(VelaScene PUBLIC)

target_link_libraries(VelaScene PUBLIC VelaMath)

vela_install_target(VelaScene
    EXPORT_NAME Scene
    INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include
)
