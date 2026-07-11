set(opengl_HEADER_FILES
        include/OpenGL/Programs/CreateProgram.hpp
        include/OpenGL/Programs/LineProgram.hpp
        include/OpenGL/Programs/MeshProgram.hpp
        include/OpenGL/Programs/ProgramId.hpp
        include/OpenGL/Programs/ProgramManager.hpp
        include/OpenGL/Programs/PointProgram.hpp
        include/OpenGL/Drawable/LineDrawable.hpp
        include/OpenGL/Drawable/MeshDrawable.hpp
        include/OpenGL/Drawable/PointDrawable.hpp
        include/OpenGL/Drawable/DrawablesManager.hpp
        include/OpenGL/Drawable/DrawableTransparencyInfo.hpp
        include/OpenGL/VertexBuffer.hpp
        include/OpenGL/BufferId.hpp
        include/OpenGL/FrameState.hpp
        include/OpenGL/BufferAccessPattern.hpp
        include/OpenGL/ErrorReporting.hpp
        include/OpenGL/GpuCapabilities.hpp
        include/OpenGL/IndexBuffer.hpp
        include/OpenGL/LineType.hpp
        include/OpenGL/OpenGL.hpp
        include/OpenGL/ShaderSources.hpp
        include/OpenGL/VertexArray.hpp
        include/OpenGL/Uniform.hpp
        include/OpenGL/Attribute.hpp
        include/OpenGL/Location.hpp
        include/OpenGL/UnsignedIntId.hpp
        include/OpenGL/UpdateBuffer.hpp
        include/OpenGL/SignedIntId.hpp
)

set(opengl_SOURCE_FILES
        source/ErrorReporting.cpp
        source/GpuCapabilities.cpp
        source/IndexBuffer.cpp
        source/FrameState.cpp
        source/Programs/CreateProgram.cpp
        source/Programs/ProgramOpenGL.cpp
        source/Programs/LineProgram.cpp
        source/Programs/MeshProgram.cpp
        source/Programs/PointProgram.cpp
        source/Programs/ProgramId.cpp
        source/Programs/ProgramOpenGL.hpp
        source/Programs/ProgramManager.cpp
        source/VertexArray.cpp
        source/VertexBuffer.cpp
        source/Drawable/LineDrawable.cpp
        source/Drawable/MeshDrawable.cpp
        source/Drawable/PointDrawable.cpp
        source/Attribute.cpp
        source/ShaderSources.cpp
        source/Uniform.cpp
)

set(TARGET_NAME plinth_opengl)
add_library(${TARGET_NAME} OBJECT ${opengl_SOURCE_FILES} ${opengl_HEADER_FILES})

include(GenerateExportHeader)
set(EXPORT_HEADER_BASE_NAME "opengl")
generate_export_header(${TARGET_NAME} BASE_NAME ${EXPORT_HEADER_BASE_NAME})
string(TOLOWER "${EXPORT_HEADER_BASE_NAME}_export.h" GENERATED_TARGET_EXPORTS_FILE_NAME)
file(COPY
        ${CMAKE_CURRENT_BINARY_DIR}/${GENERATED_TARGET_EXPORTS_FILE_NAME}
        DESTINATION
        ${CMAKE_CURRENT_BINARY_DIR}/export_header_includes/OpenGL)
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/${GENERATED_TARGET_EXPORTS_FILE_NAME})

target_include_directories(${TARGET_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/export_header_includes>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>)

target_compile_definitions(${TARGET_NAME} PUBLIC OPENGL_STATIC_DEFINE GLFW_INCLUDE_NONE)

add_library(plinth::opengl ALIAS ${TARGET_NAME})

set(TARGET_EXPORT_NAME ${TARGET_NAME})
set(${TARGET_NAME}_MAJOR 0)
set(${TARGET_NAME}_MINOR 1)
set(${TARGET_NAME}_PATCH 0)
message(STATUS "Configuring ${TARGET_NAME} ${${TARGET_NAME}_MAJOR}.${${TARGET_NAME}_MINOR}.${${TARGET_NAME}_PATCH}")

set_target_properties(
        ${TARGET_NAME}
        PROPERTIES
        LANGUAGE CXX
        POSITION_INDEPENDENT_CODE ON
        DEBUG_POSTFIX d)

add_warnings_and_compile_options(${TARGET_NAME} OFF)
target_compile_features(${TARGET_NAME} PUBLIC cxx_std_23)
if (${PROJECT_NAME}_STATIC_ANALYSIS)
    enable_static_analysis(${TARGET_NAME} OFF)
endif ()

target_link_libraries(${TARGET_NAME}
        PRIVATE
        glad::glad
        $<BUILD_LOCAL_INTERFACE:linal::linal>
)
