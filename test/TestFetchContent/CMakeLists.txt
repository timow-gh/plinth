set(test_target "plinth_fetchcontent")
set(${test_target}_TEST_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/${test_target}_test")
set(${test_target}_TEMPLATE_PROJECT_DIR_NAME "plinth_fetchcontent_test_template")

if (BUILD_SHARED_LIBS)
    set(_plinth_fetchcontent_bsl ON)
else ()
    set(_plinth_fetchcontent_bsl OFF)
endif ()

set(_plinth_fetchcontent_configure_command
        ${CMAKE_COMMAND}
        -B ${${test_target}_TEST_SOURCE_DIR}/build
        -S ${${test_target}_TEST_SOURCE_DIR}
        -DBUILD_SHARED_LIBS=${_plinth_fetchcontent_bsl}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DPLINTH_SOURCE_DIR=${PROJECT_SOURCE_DIR})

if (CMAKE_TOOLCHAIN_FILE)
    list(APPEND _plinth_fetchcontent_configure_command
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
endif ()

if (VCPKG_TARGET_TRIPLET)
    list(APPEND _plinth_fetchcontent_configure_command
            -DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET})
endif ()

set(_plinth_fetchcontent_build_command
        ${CMAKE_COMMAND}
        --build ${${test_target}_TEST_SOURCE_DIR}/build
        --config ${CMAKE_BUILD_TYPE}
        --parallel)
if (MSVC)
    list(APPEND _plinth_fetchcontent_build_command 1)
endif ()

add_custom_target(test_plinth_fetchcontent
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_LIST_DIR}/${${test_target}_TEMPLATE_PROJECT_DIR_NAME}"
        ${${test_target}_TEST_SOURCE_DIR}

        COMMAND ${_plinth_fetchcontent_configure_command}

        COMMAND ${_plinth_fetchcontent_build_command}
)

add_custom_target(test_plinth_fetchcontent_clean
        COMMAND ${CMAKE_COMMAND} -E rm -Rf ${${test_target}_TEST_SOURCE_DIR}
)

option(${PROJECT_NAME}_FETCHCONTENT_SMOKE_TEST "Register FetchContent consumer smoke test with CTest." ON)
if (${PROJECT_NAME}_FETCHCONTENT_SMOKE_TEST)
    add_test(NAME TestPlinthFetchContent COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test_plinth_fetchcontent)
    set_tests_properties(TestPlinthFetchContent PROPERTIES
            LABELS consumer
            RUN_SERIAL TRUE)
else ()
    message(STATUS "Skipping TestPlinthFetchContent CTest registration because ${PROJECT_NAME}_FETCHCONTENT_SMOKE_TEST=OFF")
endif ()
