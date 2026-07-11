#include "OpenGL/ErrorReporting.hpp"
#include "OpenGL/GpuCapabilities.hpp"
#include "OpenGL/OpenGL.hpp"

namespace opengl {

GpuCapabilities query_gpu_capabilities() {
    GpuCapabilities caps;

    glGetIntegerv(GL_MAJOR_VERSION, &caps.glMajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &caps.glMinorVersion);

    if (const auto* renderer = glGetString(GL_RENDERER)) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        caps.glRenderer = reinterpret_cast<const char*>(renderer);
    } else {
        report_warning("query_gpu_capabilities: glGetString(GL_RENDERER) returned null");
    }

    if (const auto* vendor = glGetString(GL_VENDOR)) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        caps.glVendor = reinterpret_cast<const char*>(vendor);
    } else {
        report_warning("query_gpu_capabilities: glGetString(GL_VENDOR) returned null");
    }

    if (const auto* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION)) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        caps.glslVersion = reinterpret_cast<const char*>(glslVersion);
    } else {
        report_warning("query_gpu_capabilities: glGetString(GL_SHADING_LANGUAGE_VERSION) returned null");
    }

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &caps.maxTextureSize);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &caps.maxColorAttachments);

    caps.supportsDebugOutput = caps.supports_version(4, 3);

    return caps;
}

} // namespace opengl
