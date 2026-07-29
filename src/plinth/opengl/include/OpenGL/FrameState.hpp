#ifndef OPENGL_FRAMESTATE_HPP
#define OPENGL_FRAMESTATE_HPP

#include "OpenGL/opengl_export.h"
#include "plinth/FrameState.hpp"

namespace opengl {

using renderer::ClearColor;
using renderer::ViewportRect;

OPENGL_EXPORT void begin_frame(const ClearColor& clearColor,
                               const ViewportRect& viewport,
                               bool srgbFramebuffer = true,
                               bool reversedDepth = false);

} // namespace opengl

#endif // OPENGL_FRAMESTATE_HPP
