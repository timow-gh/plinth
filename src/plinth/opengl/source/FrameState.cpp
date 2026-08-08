#include "OpenGL/FrameState.hpp"

#include "OpenGL/OpenGL.hpp"

namespace opengl {

void begin_frame(const ClearColor& clearColor, const ViewportRect& viewport, bool srgbFramebuffer, bool reversedDepth) {
    if (srgbFramebuffer) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glDepthMask(GL_TRUE);
    glClearDepth(reversedDepth ? 0.0 : 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(reversedDepth ? GL_GREATER : GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

    glViewport(static_cast<GLint>(viewport.x),
               static_cast<GLint>(viewport.y),
               static_cast<GLsizei>(viewport.width),
               static_cast<GLsizei>(viewport.height));
}

} // namespace opengl
