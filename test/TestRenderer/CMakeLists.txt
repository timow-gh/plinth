function(create_plinth_internal_test testName testLabel)
    add_executable(${testName} ${testName}.cpp)
    target_link_libraries(${testName} PRIVATE gtest gtest_main plinth::plinth)
    add_test(NAME plinth.${testName} COMMAND ${testName})
    set_tests_properties(plinth.${testName} PROPERTIES LABELS "${testLabel}")
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

# glad::glad generates code that calls dlopen/dlsym/dlclose on Linux but its
# upstream CMakeLists.txt does not declare a dependency on ${CMAKE_DL_LIBS}.
# Work around this missing transitive dependency until it is fixed upstream.
if(UNIX AND NOT APPLE AND NOT HAIKU)
    set(PLINTH_GLAD_DL_LIBS ${CMAKE_DL_LIBS})
else()
    set(PLINTH_GLAD_DL_LIBS "")
endif()

function(create_plinth_opengl_internal_test testName testLabel)
    add_executable(${testName} ${testName}.cpp)
    target_link_libraries(${testName} PRIVATE
            gtest
            gtest_main
            plinth::opengl
            glad::glad
            glfw
            linal::linal
            ${PLINTH_GLAD_DL_LIBS})
    add_test(NAME plinth.${testName} COMMAND ${testName})
    set_tests_properties(plinth.${testName} PROPERTIES LABELS "${testLabel}")
    set_tests_properties(plinth.${testName} PROPERTIES RESOURCE_LOCK plinth_glfw)
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

function(create_renderer_test testName testLabel)
    add_executable(${testName} ${testName}.cpp)
    target_link_libraries(${testName} PRIVATE
            gtest
            gtest_main
            plinth::plinth)
    add_test(NAME plinth.${testName} COMMAND ${testName})
    set_tests_properties(plinth.${testName} PROPERTIES LABELS "${testLabel}")
    if ("graphics" IN_LIST testLabel)
        set_tests_properties(plinth.${testName} PROPERTIES RESOURCE_LOCK plinth_glfw)
    endif ()
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

function(create_protocol_test testName testLabel)
    add_executable(${testName} ${testName}.cpp)
    target_link_libraries(${testName} PRIVATE gtest gtest_main plinth::plinth)
    add_test(NAME plinth.${testName} COMMAND ${testName})
    set_tests_properties(plinth.${testName} PROPERTIES LABELS "${testLabel}")
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

function(create_plinth_opengl_program_unit_test testName testLabel)
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
            glad::glad
            ${PLINTH_GLAD_DL_LIBS})
    add_test(NAME plinth.${testName} COMMAND ${testName})
    set_tests_properties(plinth.${testName} PROPERTIES LABELS "${testLabel}")
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

create_plinth_internal_test(BufferTest unit)
target_include_directories(BufferTest PRIVATE ${CMAKE_SOURCE_DIR}/src/plinth/opengl/include)
create_renderer_test(CameraAutoFitTest unit)
create_renderer_test(CameraInteractorTest unit)
create_renderer_test(CameraTest unit)
create_plinth_opengl_internal_test(DrawableTest graphics)
create_plinth_opengl_internal_test(ErrorReportingTest graphics)
create_plinth_opengl_internal_test(FrameStateTest graphics)
target_link_libraries(FrameStateTest PRIVATE plinth::plinth)
create_plinth_opengl_internal_test(FramebufferTest graphics)
create_plinth_opengl_internal_test(Texture2DTest graphics)
create_plinth_opengl_internal_test(TexturedMeshTest graphics)
target_link_libraries(TexturedMeshTest PRIVATE plinth::plinth)
create_plinth_opengl_internal_test(PresentationPassTest graphics)
create_plinth_opengl_internal_test(GoldenImageTest "graphics;golden")
target_include_directories(GoldenImageTest PRIVATE ${Stb_INCLUDE_DIR})
target_compile_definitions(GoldenImageTest PRIVATE PLINTH_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden")
create_plinth_opengl_internal_test(GpuCapabilitiesTest graphics)
create_renderer_test(InputCaptureStateTest unit)
create_renderer_test(InputStateTest graphics)
target_link_libraries(InputStateTest PRIVATE glfw)
target_include_directories(InputStateTest PRIVATE ${CMAKE_SOURCE_DIR}/src/plinth/source)
create_renderer_test(RendererViewportTest unit)
create_plinth_opengl_program_unit_test(CreateProgramTest unit)
create_renderer_test(RayPlaneIntersectionTest unit)
create_plinth_internal_test(TransparencySortingTest unit)
target_include_directories(TransparencySortingTest PRIVATE ${CMAKE_SOURCE_DIR}/src/plinth/opengl/include)
create_renderer_test(RendererTest graphics)
target_link_libraries(RendererTest PRIVATE glfw)
create_renderer_test(ViewportTest unit)
