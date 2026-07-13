#ifndef OPENGL_FRAMESTATE_HPP
#define OPENGL_FRAMESTATE_HPP

#include <plinth/FrameState.hpp>
#include <OpenGL/opengl_export.h>

namespace opengl {

using renderer::ClearColor;
using renderer::ViewportRect;

OPENGL_EXPORT void begin_frame(const ClearColor& clearColor, const ViewportRect& viewport, bool srgbFramebuffer = true);

} // namespace opengl

#endif // OPENGL_FRAMESTATE_HPP
