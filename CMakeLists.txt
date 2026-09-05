cmake_minimum_required(VERSION 3.16)
project(Vela LANGUAGES CXX)

add_compile_definitions($<$<CONFIG:Debug>:VELA_DEBUG>)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(VELA_WITH_GLFW "Build the GLFW windowing backend" ON)
option(VELA_WITH_ASSETS "Build the optional asset loading target" ON)
option(VELA_BUILD_SHARED "" OFF)
option(VELA_BUILD_EXAMPLES "" ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/Shaders.cmake)

add_subdirectory(vendor)
add_subdirectory(Foundation)
add_subdirectory(Math)
add_subdirectory(Backend)
add_subdirectory(Graphics)
add_subdirectory(Core)
add_subdirectory(Scene)
add_subdirectory(Windowing)

if(VELA_WITH_ASSETS)
    add_subdirectory(Assets)
endif()

add_library(Vela INTERFACE)
add_library(Vela::Vela ALIAS Vela)

target_link_libraries(Vela INTERFACE VelaGraphics VelaCore VelaScene VelaMath VelaFoundation)

if(VELA_WITH_GLFW)
    target_link_libraries(Vela INTERFACE VelaGLFW)
    target_link_libraries(VelaCore PUBLIC VelaGLFW)
endif()

add_library(VelaNative INTERFACE)
target_link_libraries(VelaNative INTERFACE Vela::Vela volk_headers)
add_library(Vela::Native ALIAS VelaNative)

if(VELA_BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()
