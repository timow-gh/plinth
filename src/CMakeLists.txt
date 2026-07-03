set(project_export_set_name "${PROJECT_NAME}TargetsExportSet")

add_subdirectory(plinth)

if (${PROJECT_NAME}_INSTALL)
    install(DIRECTORY "plinth/include/plinth" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            COMPONENT dev)

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/plinth/export_header_includes/plinth/plinth_export.h"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/plinth"
            COMPONENT dev)

        # Use a single install command. Multiple install commands can lead to issues with CMake's install script generation. It may only 
        # use the last install command.
    install(TARGETS
            plinth_plinth plinth_opengl
            EXPORT ${project_export_set_name}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT runtime
            LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT runtime
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT dev
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            COMPONENT dev)
endif ()

if (${PROJECT_NAME}_INSTALL)
    include(setup_export_set_installation)
    setup_export_set_installation(
            ${PROJECT_NAME}
            ${project_export_set_name}
            Config.cmake.in
            ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR} ${PROJECT_VERSION_PATCH}
    )
endif ()
