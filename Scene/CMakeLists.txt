set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)

add_library(VelaScene STATIC ${PROJECT_SOURCES})
target_include_directories(VelaScene PUBLIC include)
target_link_libraries(VelaScene PUBLIC glm::glm)
add_library(Vela::Scene ALIAS VelaScene)
target_compile_definitions(VelaScene PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)