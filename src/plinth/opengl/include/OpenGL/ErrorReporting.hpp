#ifndef OPENGL_ERRORREPORTING_HPP
#define OPENGL_ERRORREPORTING_HPP

#include <OpenGL/opengl_export.h>
#include <functional>
#include <string_view>

namespace opengl {

enum class ErrorSeverity { warning, error };

// Signature matches what a host application would want to hook into its own logger.
using ErrorSink = std::function<void(ErrorSeverity, std::string_view message)>;

// Installs a custom sink. Passing nullptr restores the default (stderr) sink.
// Not thread-safe by design - call during setup only, same as OpenGLConfig.
OPENGL_EXPORT void set_error_sink(ErrorSink sink);

// Routes to the installed sink (default: std::print(stderr, ...) with a
// "[opengl][warning|error] " prefix). Use report_error for resource-creation /
// unrecoverable-for-that-call failures, report_warning for degraded-but-continuing cases.
OPENGL_EXPORT void report_error(std::string_view message);
OPENGL_EXPORT void report_warning(std::string_view message);

// Drains glGetError() until GL_NO_ERROR, reporting each code found via report_error with
// `context` prefixed (e.g. "after glLinkProgram"). Returns true if at least one error was found.
// Only call this in debug/validation builds or at coarse granularity (once per draw call, not
// per GL call) - glGetError forces a sync point.
OPENGL_EXPORT bool check_gl_errors(std::string_view context);

} // namespace opengl

#endif // OPENGL_ERRORREPORTING_HPP
