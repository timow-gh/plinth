#ifndef LINETYPE_HPP
#define LINETYPE_HPP

#include "OpenGL/OpenGL.hpp"
#include <plinth/LineType.hpp>

namespace opengl {

using renderer::LineType;

inline unsigned int to_gl_primitive(const LineType& lineType) {
    if (lineType.is_line_strip()) {
        return GL_LINE_STRIP;
    }
    if (lineType.is_line_loop()) {
        return GL_LINE_LOOP;
    }
    return GL_LINES;
}

} // namespace opengl

#endif // LINETYPE_HPP
