set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS ${INCLUDE_DIR}/*.h ${INCLUDE_DIR}/*.hpp)
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)

add_library(VelaCore STATIC
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

add_library(Vela::Core ALIAS VelaCore)

vela_public_includes(VelaCore PUBLIC)

target_link_libraries(VelaCore
    PUBLIC
    VelaFoundation
    PRIVATE
    VelaBackend
)

vela_install_target(VelaCore
    EXPORT_NAME Core
    INCLUDE_DIR ${INCLUDE_DIR}
)
