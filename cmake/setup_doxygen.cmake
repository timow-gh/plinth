macro(enable_doxygen)
    find_program(DOXYGEN_EXECUTABLE doxygen)

    if (NOT DOXYGEN_EXECUTABLE)
        message(FATAL_ERROR "Doxygen not found. Documentation will not be generated.")
    endif ()

    set(DOXYGEN_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # @ONLY means only replace variables of the form @VAR@ (and not ${VAR}).
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in ${DOXYGEN_CONFIG_FILE} @ONLY)

    # Public headers are not yet fully documented, so retain Doxygen warnings.
    # They are emitted to the target log for visibility in CI.
    add_custom_target(
            docs
            COMMAND ${CMAKE_COMMAND} -E echo "Doxygen warnings, if any, follow in this build log."
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG_FILE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
    )
endmacro()
