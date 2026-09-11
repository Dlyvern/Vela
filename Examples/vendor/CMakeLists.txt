add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE ${CMAKE_CURRENT_LIST_DIR}/cgltf)

add_library(imgui STATIC
    ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui.cpp
    ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_demo.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/imgui
    ${CMAKE_CURRENT_LIST_DIR}/imgui/backends
)

target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK=1)

if(UNIX)
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
endif()

target_link_libraries(imgui PUBLIC volk glfw  $<$<PLATFORM_ID:Linux>:${X11_LIBRARIES}> $<$<PLATFORM_ID:Linux>:Threads::Threads>)