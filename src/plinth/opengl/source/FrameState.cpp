#include <OpenGL/FrameState.hpp>
#include <OpenGL/OpenGL.hpp>

namespace opengl {

void begin_frame(const ClearColor& clearColor, const ViewportRect& viewport) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

    glViewport(static_cast<GLint>(viewport.x),
               static_cast<GLint>(viewport.y),
               static_cast<GLsizei>(viewport.width),
               static_cast<GLsizei>(viewport.height));
}

} // namespace opengl
