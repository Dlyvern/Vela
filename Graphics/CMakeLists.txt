cmake_minimum_required(VERSION 3.16)

add_library(VelaGraphics INTERFACE)
target_include_directories(VelaGraphics INTERFACE include)
target_link_libraries(VelaGraphics INTERFACE VelaMath VelaFoundation)
add_library(Vela::Graphics ALIAS VelaGraphics)
