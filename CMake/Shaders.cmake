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

# Internal: adds a build step that copies SPV_FILES into "<directory of DESTINATION>/shaders"
# whenever one of them changes, and stores the step's stamp file in the variable named by
# OUT_STAMP. Staging is a build step of its own rather than a POST_BUILD command, so it re-runs
# after shader-only edits that don't relink DESTINATION.
function(_vela_stage_spirv DESTINATION NAME OUT_STAMP)
    set(STAMP "${CMAKE_CURRENT_BINARY_DIR}/${NAME}_$<CONFIG>.stamp")

    add_custom_command(
        OUTPUT ${STAMP}
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${DESTINATION}>/shaders"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ARGN} "$<TARGET_FILE_DIR:${DESTINATION}>/shaders"
        COMMAND ${CMAKE_COMMAND} -E touch ${STAMP}
        DEPENDS ${ARGN}
        COMMENT "Staging shaders for ${DESTINATION}"
        VERBATIM
    )

    set(${OUT_STAMP} ${STAMP} PARENT_SCOPE)
endfunction()

# Compiles GLSL shaders to SPIR-V and stages the .spv files in "<directory of TARGET>/shaders",
# where applications load them from at runtime.
# TARGET:  the target the shaders belong to; it is built after them. For a static library,
#          stage its shaders next to each executable linking it with vela_stage_shaders().
# SOURCES: GLSL files, relative to the current source directory. Files they #include are
#          tracked, so editing an include recompiles and restages every shader using it.
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
        set(SPV_DEPFILE "${SPV_OUT}.d")

        if(VELA_GLSLC)
            set(SHADER_COMMAND ${VELA_GLSLC} -MD -MF "${SPV_DEPFILE}" "${SHADER_ABS}" -o "${SPV_OUT}")
        else()
            set(SHADER_COMMAND ${VELA_GLSLANG_VALIDATOR} -V --depfile "${SPV_DEPFILE}" "${SHADER_ABS}" -o "${SPV_OUT}")
        endif()

        add_custom_command(
            OUTPUT ${SPV_OUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            COMMAND ${SHADER_COMMAND}
            DEPENDS "${SHADER_ABS}"
            DEPFILE "${SPV_DEPFILE}"
            COMMENT "Compiling ${SHADER}"
            VERBATIM
        )

        list(APPEND SPV_OUTPUTS ${SPV_OUT})
    endforeach()

    set_property(TARGET ${ARG_TARGET} APPEND PROPERTY VELA_SPIRV_FILES ${SPV_OUTPUTS})

    _vela_stage_spirv(${ARG_TARGET} ${ARG_TARGET}_shaders_staged STAGE_STAMP ${SPV_OUTPUTS})

    add_custom_target(${ARG_TARGET}_shaders DEPENDS ${SPV_OUTPUTS} ${STAGE_STAMP})
    add_dependencies(${ARG_TARGET} ${ARG_TARGET}_shaders)
endfunction()

# Stages the shaders another target compiled with vela_compile_shaders() in
# "<directory of TARGET>/shaders". Use it for executables linking a static library that loads
# its own shaders at runtime: the library's shaders are compiled once and copied next to each.
# TARGET: the executable (or shared library) that needs the shaders at runtime.
# FROM:   the target whose vela_compile_shaders() call compiled them; that call must come first.
function(vela_stage_shaders)
    cmake_parse_arguments(ARG "" "TARGET;FROM" "" ${ARGN})

    if(NOT ARG_TARGET OR NOT ARG_FROM)
        message(FATAL_ERROR "vela_stage_shaders: TARGET and FROM are required")
    endif()

    get_target_property(SPV_FILES ${ARG_FROM} VELA_SPIRV_FILES)

    if(NOT SPV_FILES)
        message(FATAL_ERROR
            "vela_stage_shaders(${ARG_TARGET}): ${ARG_FROM} has no shaders; call "
            "vela_compile_shaders(TARGET ${ARG_FROM} ...) before this."
        )
    endif()

    set(STAGE_NAME ${ARG_TARGET}_${ARG_FROM}_shaders)
    _vela_stage_spirv(${ARG_TARGET} ${STAGE_NAME} STAGE_STAMP ${SPV_FILES})

    add_custom_target(${STAGE_NAME} DEPENDS ${STAGE_STAMP})
    add_dependencies(${STAGE_NAME} ${ARG_FROM}_shaders)
    add_dependencies(${ARG_TARGET} ${STAGE_NAME})
endfunction()
