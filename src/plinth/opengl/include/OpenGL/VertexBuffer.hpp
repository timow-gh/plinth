#ifndef OPENGL_VERTEXBUFFER_HPP
#define OPENGL_VERTEXBUFFER_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/BufferId.hpp"
#include "OpenGL/Location.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/UnsignedIntId.hpp"
#include "OpenGL/opengl_export.h"
#include <cstddef>
#include <optional>
#include <span>

namespace opengl {

/** @brief An opengl buffer object. */
class OPENGL_EXPORT VertexBuffer {
    std::optional<BufferId> m_bufferId;
    GLsizei m_vectorDimension{0};
    GLsizei m_stride{0};
    Location m_bufferLocation;

  public:
    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;
    VertexBuffer(VertexBuffer&& other) noexcept;
    VertexBuffer& operator=(VertexBuffer&& other) noexcept;
    ~VertexBuffer();

    void reset() noexcept;

    static std::optional<VertexBuffer> create(std::span<const float> vectors,
                                              GLsizei vectorDimension,
                                              Location bufferLocation,
                                              BufferAccessPattern accessPattern);

    [[nodiscard]]
    const BufferId& get_buffer_id() const;

    void bind() const;

    static void unbind();

    void update_buffer(std::span<const float> vertices, BufferAccessPattern accessPattern);

  private:
    VertexBuffer(BufferId bufferId, GLsizei vectorDimension, GLsizei stride, Location bufferLocation);
};

} // namespace opengl

#endif // OPENGL_VERTEXBUFFER_HPP
