#ifndef OPENGL_OPENGL_HPP
#define OPENGL_OPENGL_HPP

#include <OpenGL/opengl_export.h>
#include <Renderer/DLLWarnings.hpp>
#include <glad/glad.h>
#include <string_view>

namespace opengl {

RENDERER_SUPPRESS_STL_DLL_WARNINGS_BEGIN

struct OPENGL_EXPORT OpenGLConfig {
    int majorVersion = 3;
    int minorVersion = 3;
    std::string_view glsl_version = "#version 330";
    bool debug{false};
};

RENDERER_SUPPRESS_STL_DLL_WARNINGS_END

} // namespace opengl

#endif // OPENGL_OPENGL_HPP
