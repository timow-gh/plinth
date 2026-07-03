#ifndef OPENGL_FRAMESTATE_HPP
#define OPENGL_FRAMESTATE_HPP

#include <OpenGL/opengl_export.h>

namespace opengl {

struct ClearColor {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{1.0f};
};

struct ViewportRect {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

OPENGL_EXPORT void begin_frame(const ClearColor& clearColor, const ViewportRect& viewport);

} // namespace opengl

#endif // OPENGL_FRAMESTATE_HPP
