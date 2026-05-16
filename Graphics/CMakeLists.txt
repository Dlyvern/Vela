cmake_minimum_required(VERSION 3.16)

add_library(VelaGraphics INTERFACE)
target_include_directories(VelaGraphics INTERFACE include)
target_link_libraries(VelaGraphics INTERFACE glm::glm)
add_library(Vela::Graphics ALIAS VelaGraphics)
target_compile_definitions(VelaGraphics INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE)