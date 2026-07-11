#ifndef OPENGL_UPDATEBUFFER_HPP
#define OPENGL_UPDATEBUFFER_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include <plinth/Assert.hpp>
#include <cstddef>
#include <limits>
#include <span>

namespace opengl {

/** @brief Updates a buffer using buffer orphaning.
 *
 * This function binds the buffer, allocates new memory for it, and fills it with the new data.
 */
template <typename TBuffer, typename T>
void update_buffer(TBuffer& buffer, std::span<const T> newBufferData, opengl::BufferAccessPattern accessPattern) {
    glBindBuffer(GL_ARRAY_BUFFER, buffer.get_buffer_id().get_value());
    const auto size = newBufferData.size() * sizeof(T);
    RENDERER_ASSERT(size <= static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max()));
    const auto bufferSize = static_cast<GLsizeiptr>(size);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, get_enum_value(accessPattern));
    glBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, newBufferData.data());
}

} // namespace opengl

#endif // OPENGL_UPDATEBUFFER_HPP
