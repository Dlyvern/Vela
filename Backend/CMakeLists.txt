set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS ${SRC_DIR}/*.h ${SRC_DIR}/*.hpp)
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)

add_library(VelaBackend STATIC
        ${PROJECT_SOURCES}
        ${PROJECT_HEADERS}
)

add_library(Vela::Backend ALIAS VelaBackend)

target_include_directories(VelaBackend PRIVATE
        ${SRC_DIR}
        ${CMAKE_SOURCE_DIR}/Core/include
        ${CMAKE_SOURCE_DIR}/Graphics/include
)

target_link_libraries(VelaBackend
    PRIVATE
    glfw
    volk_headers
    VulkanMemoryAllocator
    volk
    VelaMath
    VelaFoundation
)

vela_install_target(VelaBackend EXPORT_NAME Backend)
