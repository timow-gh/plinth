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

// Enables GL_DEBUG_OUTPUT (synchronous) and installs a glDebugMessageCallback that routes
// driver-provided diagnostics through report_error/report_warning. Requires a GL 4.3+ context
// (glDebugMessageCallback is core since 4.3, promoted from KHR_debug/ARB_debug_output) - the
// availability check queries the actual context version via glGetIntegerv(GL_MAJOR_VERSION /
// GL_MINOR_VERSION) rather than relying on GLAD_GL_VERSION_* macros which reflect driver
// capabilities, not the context version. Returns false without side effects if the current
// context is below 4.3. Call after gladLoadGLLoader has populated function pointers.
OPENGL_EXPORT bool install_debug_callback();

} // namespace opengl

#endif // OPENGL_ERRORREPORTING_HPP
