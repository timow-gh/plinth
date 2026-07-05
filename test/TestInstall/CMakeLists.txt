set(test_target "plinth")
set(${test_target}_TEST_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/${test_target}_install_dir")
set(${test_target}_TEST_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/${test_target}_use_install_test")
set(${test_target}_TEMPLATE_PROJECT_DIR_NAME "${test_target}_install_test_template")

set(PRESET "")
if (MSVC)
    set(PRESET "conf-windows-msvc-debug")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(PRESET "conf-unixlike-clang-debug")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(PRESET "conf-unixlike-gcc-debug")
else ()
    message(AUTHOR_WARNING "Unsupported compiler")
endif ()

# Normalize to an explicit ON/OFF so the inner configure's --preset default
# (which sets BUILD_SHARED_LIBS=ON) is always overridden by the outer build's
# actual setting, even when the outer build leaves BUILD_SHARED_LIBS unset
# (empty string).
if (BUILD_SHARED_LIBS)
    set(_plinth_test_install_bsl ON)
else ()
    set(_plinth_test_install_bsl OFF)
endif ()

add_custom_target(test_geoqik_install
        ${CMAKE_COMMAND} --install ${PROJECT_BINARY_DIR} --config ${CMAKE_BUILD_TYPE} --prefix ${${test_target}_TEST_INSTALL_DIR}

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_LIST_DIR}/${${test_target}_TEMPLATE_PROJECT_DIR_NAME}"
        ${${test_target}_TEST_SOURCE_DIR}

        COMMAND ${CMAKE_COMMAND}
        -B ${${test_target}_TEST_SOURCE_DIR}/build
        -S ${${test_target}_TEST_SOURCE_DIR}
        --preset "${PRESET}"
        # Test with the same BUILD_SHARED_LIBS value as the main project
        -DBUILD_SHARED_LIBS=${_plinth_test_install_bsl}
        -DCMAKE_PREFIX_PATH=${${test_target}_TEST_INSTALL_DIR}

        COMMAND ${CMAKE_COMMAND}
        --build ${${test_target}_TEST_SOURCE_DIR}/build
        --config ${CMAKE_BUILD_TYPE}
        --parallel

        COMMAND ${CMAKE_COMMAND}
        --install ${${test_target}_TEST_SOURCE_DIR}/build
        --config ${CMAKE_BUILD_TYPE}
        --prefix ${${test_target}_TEST_SOURCE_DIR}/install
)

add_custom_target(test_geoqik_install_clean
        ${CMAKE_COMMAND} -E rm -Rf ${${test_target}_TEST_INSTALL_DIR}
        COMMAND ${CMAKE_COMMAND} -E rm -Rf ${${test_target}_TEST_SOURCE_DIR}
)

# CMAKE_BINARY_DIR is the root build directory. Using an absolute path here
# is required because CTest's default working directory for a test is the
# subdirectory build tree (out/build/<preset>/test/TestInstall/), which has no
# build.ninja. cmake --build then calls ninja in the wrong directory and fails
# immediately with "ninja: error: loading 'build.ninja': No such file or directory".
option(${PROJECT_NAME}_INSTALL_SMOKE_TEST "Register installed package smoke test with CTest." ON)
if (${PROJECT_NAME}_INSTALL_SMOKE_TEST)
    add_test(NAME TestGeoqikInstall COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test_geoqik_install)
else ()
    message(STATUS "Skipping TestGeoqikInstall CTest registration because ${PROJECT_NAME}_INSTALL_SMOKE_TEST=OFF")
endif ()
