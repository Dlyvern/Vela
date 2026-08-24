# Locate a GLSL -> SPIR-V compiler.
#
find_program(VELA_GLSLC
    NAMES glslc
    HINTS ENV VULKAN_SDK
    PATH_SUFFIXES Bin bin
    DOC "glslc GLSL->SPIR-V compiler (ships with the Vulkan SDK)"
)

find_program(VELA_GLSLANG_VALIDATOR
    NAMES glslangValidator glslang
    HINTS ENV VULKAN_SDK
    PATH_SUFFIXES Bin bin
    DOC "glslang reference compiler, used as a fallback when glslc is missing"
)

if(VELA_GLSLC)
    message(STATUS "Vela: shader compiler: ${VELA_GLSLC}")
elseif(VELA_GLSLANG_VALIDATOR)
    message(STATUS "Vela: shader compiler: ${VELA_GLSLANG_VALIDATOR} (glslc not found)")
else()
    message(STATUS "Vela: no GLSL compiler found - shader compilation is unavailable")
endif()

function(vela_compile_shaders)
    cmake_parse_arguments(ARG "" "TARGET" "SOURCES" ${ARGN})

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "vela_compile_shaders: TARGET is required")
    endif()

    if(NOT VELA_GLSLC AND NOT VELA_GLSLANG_VALIDATOR)
        message(FATAL_ERROR
            "vela_compile_shaders(${ARG_TARGET}): no GLSL compiler found.\n"
            "Install the Vulkan SDK (https://vulkan.lunarg.com) so that glslc is\n"
            "available, or point CMake at it directly with -DVELA_GLSLC=<path/to/glslc>.\n"
            "On Windows the SDK installs it as %VULKAN_SDK%\\Bin\\glslc.exe."
        )
    endif()

    set(SPV_OUTPUTS "")
    foreach(SHADER ${ARG_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        get_filename_component(SHADER_ABS "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}" ABSOLUTE)
        set(SPV_OUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv")

        if(VELA_GLSLC)
            set(SHADER_COMMAND ${VELA_GLSLC} "${SHADER_ABS}" -o "${SPV_OUT}")
        else()
            set(SHADER_COMMAND ${VELA_GLSLANG_VALIDATOR} -V "${SHADER_ABS}" -o "${SPV_OUT}")
        endif()

        add_custom_command(
            OUTPUT ${SPV_OUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            COMMAND ${SHADER_COMMAND}
            DEPENDS "${SHADER_ABS}"
            COMMENT "Compiling ${SHADER}"
            VERBATIM
        )

        list(APPEND SPV_OUTPUTS ${SPV_OUT})
    endforeach()

    # Make the target depend on all .spv files, so they rebuild on edit
    add_custom_target(${ARG_TARGET}_shaders DEPENDS ${SPV_OUTPUTS})
    add_dependencies(${ARG_TARGET} ${ARG_TARGET}_shaders)

    # Multi-config generators (Visual Studio, Xcode) put the executable in a
    # per-config subdirectory, so the binary dir is not where the runtime looks
    # for "shaders/*.spv". Mirror them next to the executable.
    add_custom_command(TARGET ${ARG_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${ARG_TARGET}>/shaders"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SPV_OUTPUTS} "$<TARGET_FILE_DIR:${ARG_TARGET}>/shaders"
        COMMENT "Staging shaders for ${ARG_TARGET}"
        VERBATIM
    )
endfunction()
