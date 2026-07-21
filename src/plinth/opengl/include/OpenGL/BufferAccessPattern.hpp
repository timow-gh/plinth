#ifndef OPENGL_BUFFERACCESSPATTERN_HPP
#define OPENGL_BUFFERACCESSPATTERN_HPP

#include "OpenGL/OpenGL.hpp"
#include <plinth/BufferAccessPattern.hpp>

namespace opengl {

using renderer::BufferAccessPattern;

constexpr unsigned int get_gl_buffer_usage(BufferAccessPattern accessPattern) {
    switch (accessPattern) {
    case BufferAccessPattern::Stream:  return GL_STREAM_DRAW;
    case BufferAccessPattern::Static:  return GL_STATIC_DRAW;
    case BufferAccessPattern::Dynamic: return GL_DYNAMIC_DRAW;
    }
    return GL_STATIC_DRAW;
}

} // namespace opengl

#endif // OPENGL_BUFFERACCESSPATTERN_HPP
