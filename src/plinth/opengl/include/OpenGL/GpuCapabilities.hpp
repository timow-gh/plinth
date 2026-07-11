#ifndef OPENGL_GPUCAPABILITIES_HPP
#define OPENGL_GPUCAPABILITIES_HPP

#include <OpenGL/opengl_export.h>
#include <plinth/DLLWarnings.hpp>
#include <string>

namespace opengl {

RENDERER_SUPPRESS_STL_DLL_WARNINGS_BEGIN

// Snapshot of what the current GL context/driver actually granted, queried once right after
// context creation. Downstream code (future framebuffer/post-processing work, future materials
// work) consults this to pick between code paths instead of hardcoding version assumptions or
// scattering ad hoc GLAD_GL_VERSION_x_y checks through unrelated files.
struct OPENGL_EXPORT GpuCapabilities {
    int glMajorVersion{0};
    int glMinorVersion{0};
    std::string glRenderer;     // GL_RENDERER, e.g. "NVIDIA GeForce RTX 3070/PCIe/SSE2"
    std::string glVendor;       // GL_VENDOR
    std::string glslVersion;    // GL_SHADING_LANGUAGE_VERSION, e.g. "4.30"
    int maxTextureSize{0};      // GL_MAX_TEXTURE_SIZE
    int maxColorAttachments{0}; // GL_MAX_COLOR_ATTACHMENTS - relevant once framebuffer work lands
    bool supportsDebugOutput{false}; // core-4.3 glDebugMessageCallback path (see ErrorReporting.hpp)

    // Convenience: true if the context is at least the given version.
    [[nodiscard]] bool supports_version(int major, int minor) const noexcept {
        return glMajorVersion > major || (glMajorVersion == major && glMinorVersion >= minor);
    }
};

RENDERER_SUPPRESS_STL_DLL_WARNINGS_END

// Queries the *current* GL context (must already be made current) - call this right after
// gladLoadGLLoader succeeds, same place GlfwWindow::create already does its post-load setup.
// Safe to call multiple times; cheap (a handful of glGetString/glGetIntegerv calls), but there's
// no reason to call it more than once per context - cache the result (see GlfwWindow/Renderer).
[[nodiscard]] OPENGL_EXPORT GpuCapabilities query_gpu_capabilities();

} // namespace opengl

#endif // OPENGL_GPUCAPABILITIES_HPP
