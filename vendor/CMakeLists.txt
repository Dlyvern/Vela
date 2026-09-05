# glfw
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Disable GLFW examples" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Disable GLFW tests" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Disable GLFW docs" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Disable GLFW install target" FORCE)
set(GLFW_LIBRARY_TYPE STATIC CACHE STRING "Build GLFW as a static library" FORCE)
add_subdirectory(glfw)

# volk
set(VOLK_INSTALL OFF CACHE BOOL "Disable volk install target" FORCE)
add_subdirectory(volk)

# VulkanMemoryAllocator
set(VMA_ENABLE_INSTALL OFF CACHE BOOL "Disable VMA install target" FORCE)
set(VMA_BUILD_DOCUMENTATION OFF CACHE BOOL "Disable VMA docs" FORCE)
set(VMA_BUILD_SAMPLES OFF CACHE BOOL "Disable VMA samples" FORCE)
add_subdirectory(VulkanMemoryAllocator)

#stb
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)
