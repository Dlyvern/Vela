cmake_minimum_required(VERSION 3.21)
project(Vela VERSION 0.1.0 LANGUAGES CXX)

add_compile_definitions($<$<CONFIG:Debug>:VELA_DEBUG>)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(VELA_WITH_GLFW "Build the GLFW windowing backend" ON)
option(VELA_WITH_ASSETS "Build the optional asset loading target" ON)
option(VELA_BUILD_SHARED "" OFF)
option(VELA_BUILD_EXAMPLES "" ON)
option(VELA_WITH_BUILTINS "" ON)
option(VELA_INSTALL "Generate install and export rules for Vela" ${PROJECT_IS_TOP_LEVEL})

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/Shaders.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/Install.cmake)

find_package(Vulkan REQUIRED)

add_subdirectory(vendor)
add_subdirectory(Foundation)
add_subdirectory(Math)
add_subdirectory(Backend)
add_subdirectory(Graphics)
add_subdirectory(Core)
add_subdirectory(Windowing)

if(VELA_WITH_BUILTINS)
    add_subdirectory(Builtins)
endif()

if(VELA_WITH_ASSETS)
    add_subdirectory(Assets)
endif()

add_library(Vela INTERFACE)
add_library(Vela::Vela ALIAS Vela)

target_link_libraries(Vela INTERFACE VelaGraphics VelaCore VelaMath VelaFoundation)

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

if(VELA_INSTALL)
    set_target_properties(Vela PROPERTIES EXPORT_NAME Vela)
    set_target_properties(VelaNative PROPERTIES EXPORT_NAME Native)

    install(TARGETS Vela VelaNative EXPORT VelaTargets)

    install(EXPORT VelaTargets
        FILE VelaTargets.cmake
        NAMESPACE Vela::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Vela
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/CMake/VelaConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/VelaConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Vela
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/VelaConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/VelaConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/VelaConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Vela
    )
endif()
