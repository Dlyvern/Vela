function(vela_install_target target)
    if(NOT VELA_INSTALL)
        return()
    endif()

    cmake_parse_arguments(ARG "" "EXPORT_NAME;INCLUDE_DIR" "" ${ARGN})

    if(NOT ARG_EXPORT_NAME)
        message(FATAL_ERROR "vela_install_target(${target}): EXPORT_NAME is required")
    endif()

    set_target_properties(${target} PROPERTIES EXPORT_NAME ${ARG_EXPORT_NAME})

    install(TARGETS ${target}
        EXPORT VelaTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    if(ARG_INCLUDE_DIR)
        install(DIRECTORY ${ARG_INCLUDE_DIR}/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif()
endfunction()

function(vela_public_includes target)
    target_include_directories(${target} ${ARGN}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
endfunction()
