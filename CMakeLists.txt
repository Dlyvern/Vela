cmake_minimum_required(VERSION 3.16)
project(Vela LANGUAGES CXX)

add_compile_definitions($<$<CONFIG:Debug>:VELA_DEBUG>)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(VELA_WITH_GLFW "Build the GLFW windowing backend" ON)
option(VELA_BUILD_SHARED "" OFF)
option(VELA_BUILD_EXAMPLES "" ON)

find_program(GLSLC glslc REQUIRED)

include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/Shaders.cmake)

add_subdirectory(vendor)
add_subdirectory(Utilities)
add_subdirectory(Backend)
add_subdirectory(Graphics)
add_subdirectory(Core)
add_subdirectory(Scene)
add_subdirectory(Windowing)

add_library(Vela INTERFACE)
add_library(Vela::Vela ALIAS Vela)

target_link_libraries(Vela INTERFACE VelaGraphics VelaCore VelaScene)

add_library(VelaNative INTERFACE)
target_link_libraries(VelaNative INTERFACE Vela::Vela volk_headers)
add_library(Vela::Native ALIAS VelaNative)

if(VELA_BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()
