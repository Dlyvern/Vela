function(vela_compile_shaders)
    cmake_parse_arguments(ARG "" "TARGET" "SOURCES" ${ARGN})

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "vela_compile_shaders: TARGET is required")
    endif()

    set(SPV_OUTPUTS "")
    foreach(SHADER ${ARG_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SPV_OUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv")

        add_custom_command(
            OUTPUT ${SPV_OUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            COMMAND ${GLSLC} ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER} -o ${SPV_OUT}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
            COMMENT "Compiling ${SHADER}"
            VERBATIM
        )

        list(APPEND SPV_OUTPUTS ${SPV_OUT})
    endforeach()

    # Make the target depend on all .spv files, so they rebuild on edit
    add_custom_target(${ARG_TARGET}_shaders DEPENDS ${SPV_OUTPUTS})
    add_dependencies(${ARG_TARGET} ${ARG_TARGET}_shaders)
endfunction()
