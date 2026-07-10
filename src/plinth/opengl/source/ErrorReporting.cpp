#include "OpenGL/ErrorReporting.hpp"
#include "OpenGL/OpenGL.hpp"
#include <format>
#include <print>
#include <utility>

namespace opengl {

namespace {

ErrorSink& active_sink() {
    static ErrorSink sink;
    return sink;
}

void default_sink(ErrorSeverity severity, std::string_view message) {
    std::print(stderr, "[opengl][{}] {}\n", severity == ErrorSeverity::error ? "error" : "warning", message);
}

void dispatch(ErrorSeverity severity, std::string_view message) {
    const ErrorSink& sink = active_sink();
    if (sink) {
        sink(severity, message);
    } else {
        default_sink(severity, message);
    }
}

std::string_view gl_error_name(GLenum err) {
    switch (err) {
    case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
    default:                               return {};
    }
}

void GLAPIENTRY on_gl_debug_message(GLenum /*source*/,
                                    GLenum type,
                                    GLuint /*id*/,
                                    GLenum severity,
                                    GLsizei length,
                                    const GLchar* message,
                                    const void* /*userParam*/) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }
    const ErrorSeverity mapped = (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM)
                                      ? ErrorSeverity::error
                                      : ErrorSeverity::warning;
    const std::string_view text{message, static_cast<std::size_t>(length)};
    if (mapped == ErrorSeverity::error) {
        report_error(std::format("[gl-debug type=0x{:04X}] {}", type, text));
    } else {
        report_warning(std::format("[gl-debug type=0x{:04X}] {}", type, text));
    }
}

} // namespace

void set_error_sink(ErrorSink sink) {
    active_sink() = std::move(sink);
}

void report_error(std::string_view message) {
    dispatch(ErrorSeverity::error, message);
}

void report_warning(std::string_view message) {
    dispatch(ErrorSeverity::warning, message);
}

bool check_gl_errors(std::string_view context) {
    bool found = false;
    for (GLenum err = glGetError(); err != GL_NO_ERROR; err = glGetError()) {
        found = true;
        const std::string_view name = gl_error_name(err);
        if (name.empty()) {
            report_error(std::format("{}: GL error 0x{:04X}", context, err));
        } else {
            report_error(std::format("{}: GL error {} (0x{:04X})", context, name, err));
        }
    }
    return found;
}

bool install_debug_callback() {
    if (!GLAD_GL_VERSION_4_3) {
        return false;
    }
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(on_gl_debug_message, nullptr);
    return true;
}

} // namespace opengl
