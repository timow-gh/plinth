cmake_minimum_required(VERSION 3.21) # CMakePresets are supported since version 3.21

project(use_installdir_proj
        VERSION 0.1.0
        LANGUAGES CXX)

set(${TARGET_NAME}_MAJOR 0)
set(${TARGET_NAME}_MINOR 1)
set(${TARGET_NAME}_PATCH 0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
message(STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
include(${PROJECT_SOURCE_DIR}/cmake/project_preamble.cmake)

project_preamble()

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Find the installed package
find_package(plinth CONFIG REQUIRED)

set(TARGET_NAME use_installdir)
# Use the installed package
add_executable(${TARGET_NAME} main.cpp)
target_link_libraries(${TARGET_NAME} PRIVATE plinth::plinth)

set_target_properties(${TARGET_NAME}
        PROPERTIES
        LANGUAGE CXX
        LINKER_LANGUAGE CXX
        DEBUG_POSTFIX d
        VERSION ${${TARGET_NAME}_MAJOR}.${${TARGET_NAME}_MINOR}.${${TARGET_NAME}_PATCH}
        SOVERSION ${PROJECT_VERSION_MAJOR})

if (BUILD_SHARED_LIBS)
    if (MSVC)
        # Copy DLLs to build directory for development
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${TARGET_NAME}> $<TARGET_FILE_DIR:${TARGET_NAME}>
                COMMAND_EXPAND_LISTS
        )

        # Install executable and collect dependencies
        install(TARGETS ${TARGET_NAME}
                RUNTIME_DEPENDENCY_SET appDeps
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

        # Install all runtime dependencies, excluding system libraries
        install(RUNTIME_DEPENDENCY_SET appDeps
                PRE_EXCLUDE_REGEXES
                [[api-ms-win-.*]] [[ext-ms-.*]] [[kernel32\.dll]] [[msvc.*]] [[vcruntime.*]] [[ucrt.*]]
                [[ntdll\.dll]] [[advapi32\.dll]] [[user32\.dll]] [[gdi32\.dll]] [[shell32\.dll]]
                [[HvsiFileTrust\.dll]] [[bcrypt\.dll]] [[crypt32\.dll]]
                # wtdccm.dll is a Windows CI runner monitoring component that
                # appears in PE import tables on GitHub-hosted runners but is
                # not a redistributable dependency.
                [[wtdccm.*]]
                POST_EXCLUDE_REGEXES
                [[.*/[Ss]ystem32/.*]]
                [[.*/[Ss]ys[Ww][Oo][Ww]64/.*]]
                [[.*/[Ww]indows/.*]]
                [[.*[Ww]indows[Kk]its.*]]
                [[.*[Mm]icrosoft.*]]
                DIRECTORIES "${CMAKE_PREFIX_PATH}/bin"
                DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
else()
    install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()


