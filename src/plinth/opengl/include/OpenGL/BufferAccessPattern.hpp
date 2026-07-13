#ifndef OPENGL_BUFFERACCESSPATTERN_HPP
#define OPENGL_BUFFERACCESSPATTERN_HPP

#include <plinth/BufferAccessPattern.hpp>

namespace opengl {

using renderer::BufferAccessPattern;

constexpr auto get_enum_value(BufferAccessPattern accessPattern) {
    return static_cast<unsigned int>(accessPattern);
}

} // namespace opengl

#endif // OPENGL_BUFFERACCESSPATTERN_HPP
