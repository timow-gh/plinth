#include "OpenGL/GpuCapabilities.hpp"
#include "OpenGL/ErrorReporting.hpp"
#include "OpenGL/OpenGL.hpp"
#include <algorithm>
#include <cstring>

namespace opengl {
namespace {

constexpr int clipControlCoreMajor = 4;
constexpr int clipControlCoreMinor = 5;

bool has_extension(const char* requestedExtension) {
    GLint extensionCount = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
    for (GLint index = 0; index < extensionCount; ++index) {
        const auto* extension = glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(index));
        if (extension != nullptr &&
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            std::strcmp(reinterpret_cast<const char*>(extension), requestedExtension) == 0) {
            return true;
        }
    }
    return false;
}

} // namespace

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
    glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &caps.maxAnisotropy);
    caps.maxAnisotropy = std::max(caps.maxAnisotropy, 1);

    caps.supportsDebugOutput = caps.supports_version(4, 3);
    caps.supportsClipControl = glad_glClipControl != nullptr &&
                               (caps.supports_version(clipControlCoreMajor, clipControlCoreMinor) ||
                                has_extension("GL_ARB_clip_control"));

    return caps;
}

} // namespace opengl
