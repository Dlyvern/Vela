# glfw
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Disable GLFW examples" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Disable GLFW tests" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Disable GLFW docs" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Disable GLFW install target" FORCE)
set(GLFW_LIBRARY_TYPE STATIC CACHE STRING "Build GLFW as a static library" FORCE)
add_subdirectory(glfw)

target_compile_definitions(glfw INTERFACE GLFW_INCLUDE_NONE)

# volk
set(VOLK_INSTALL OFF CACHE BOOL "Disable volk install target" FORCE)
set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "Vela supplies the Vulkan headers" FORCE)
add_subdirectory(volk)

target_link_libraries(volk PUBLIC Vulkan::Headers)
target_link_libraries(volk_headers INTERFACE Vulkan::Headers)

# VulkanMemoryAllocator
set(VMA_ENABLE_INSTALL OFF CACHE BOOL "Disable VMA install target" FORCE)
set(VMA_BUILD_DOCUMENTATION OFF CACHE BOOL "Disable VMA docs" FORCE)
set(VMA_BUILD_SAMPLES OFF CACHE BOOL "Disable VMA samples" FORCE)
add_subdirectory(VulkanMemoryAllocator)

#stb
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/stb>)

if(VELA_INSTALL)
    install(TARGETS glfw volk volk_headers VulkanMemoryAllocator stb_image
        EXPORT VelaTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/volk/volk.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
