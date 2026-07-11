function(create_plinth_internal_test testName)
    add_executable(${testName} ${testName}.cpp)
    target_link_libraries(${testName} PRIVATE gtest gtest_main plinth::plinth)
    add_test(NAME plinth.${testName} COMMAND ${testName})
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

function(create_plinth_opengl_internal_test testName)
    add_executable(${testName} ${testName}.cpp)
    target_link_libraries(${testName} PRIVATE
            gtest
            gtest_main
            plinth::opengl
            glad::glad
            glfw
            linal::linal)
    add_test(NAME plinth.${testName} COMMAND ${testName})
    set_tests_properties(plinth.${testName} PROPERTIES RESOURCE_LOCK plinth_glfw)
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

function(create_renderer_test testName)
    add_executable(${testName} ${testName}.cpp)
    target_link_libraries(${testName} PRIVATE
            gtest
            gtest_main
            plinth::plinth)
    add_test(NAME plinth.${testName} COMMAND ${testName})
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

function(create_protocol_test testName)
    add_executable(${testName} ${testName}.cpp)
    target_link_libraries(${testName} PRIVATE gtest gtest_main plinth::plinth)
    add_test(NAME plinth.${testName} COMMAND ${testName})
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

function(create_plinth_opengl_program_unit_test testName)
    add_executable(${testName}
            ${testName}.cpp
            ${CMAKE_SOURCE_DIR}/src/plinth/opengl/source/Programs/CreateProgram.cpp
            ${CMAKE_SOURCE_DIR}/src/plinth/opengl/source/Programs/ProgramId.cpp
            ${CMAKE_SOURCE_DIR}/src/plinth/opengl/source/ErrorReporting.cpp)
    target_include_directories(${testName} PRIVATE
            ${CMAKE_BINARY_DIR}/src/plinth/opengl/export_header_includes
            ${CMAKE_SOURCE_DIR}/src/plinth/opengl/include
            ${CMAKE_SOURCE_DIR}/src/plinth/include
            ${CMAKE_SOURCE_DIR}/src/plinth/opengl/source)
    target_compile_definitions(${testName} PRIVATE OPENGL_STATIC_DEFINE)
    target_compile_features(${testName} PRIVATE cxx_std_23)
    target_link_libraries(${testName} PRIVATE
            gmock
            gtest_main
            fmt::fmt
            glad::glad)
    add_test(NAME plinth.${testName} COMMAND ${testName})
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

create_plinth_internal_test(BufferTest)
create_renderer_test(CameraAutoFitTest)
create_renderer_test(CameraInteractorTest)
create_renderer_test(CameraTest)
create_plinth_opengl_internal_test(DrawableTest)
create_plinth_opengl_internal_test(ErrorReportingTest)
create_plinth_opengl_internal_test(GoldenImageTest)
target_include_directories(GoldenImageTest PRIVATE ${Stb_INCLUDE_DIR})
target_compile_definitions(GoldenImageTest PRIVATE PLINTH_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden")
create_plinth_opengl_internal_test(GpuCapabilitiesTest)
create_renderer_test(InputCaptureStateTest)
create_renderer_test(InputStateTest)
set_tests_properties(plinth.InputStateTest PROPERTIES RESOURCE_LOCK plinth_glfw)
create_renderer_test(RendererViewportTest)
create_plinth_opengl_program_unit_test(CreateProgramTest)
create_renderer_test(RayPlaneIntersectionTest)
create_plinth_internal_test(TransparencySortingTest)
create_renderer_test(ViewportTest)
