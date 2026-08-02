#ifndef OPENGL_INSTANCEBUFFER_HPP
#define OPENGL_INSTANCEBUFFER_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/BufferId.hpp"
#include "OpenGL/Location.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/opengl_export.h"
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace opengl {

/** @brief Describes one float attribute view into an interleaved per-instance buffer. */
struct InstanceAttribSpec {
    Location location;
    GLint componentCount{0}; // number of floats (1..4)
    GLsizei offsetBytes{0};  // byte offset of this attribute within the per-instance stride
};

/** @brief A per-instance GL buffer for instanced rendering.
 *
 * Unlike VertexBuffer (single float attribute, offset 0), InstanceBuffer holds one interleaved
 * float blob that feeds several attributes, each configured with an attribute divisor of 1 so it
 * advances once per instance rather than per vertex. Like VertexBuffer, bind() re-applies the full
 * attribute setup (pointer and divisor) because the renderer reconfigures attributes on every bind
 * rather than relying on sticky VAO state.
 */
class OPENGL_EXPORT InstanceBuffer {
    std::optional<BufferId> m_bufferId;
    GLsizei m_strideBytes{0};
    GLsizei m_instanceCount{0};
    std::vector<InstanceAttribSpec> m_attribs;

  public:
    InstanceBuffer() = default;
    InstanceBuffer(const InstanceBuffer&) = delete;
    InstanceBuffer& operator=(const InstanceBuffer&) = delete;
    InstanceBuffer(InstanceBuffer&& other) noexcept;
    InstanceBuffer& operator=(InstanceBuffer&& other) noexcept;
    ~InstanceBuffer();

    void reset() noexcept;

    /// Creates the buffer and configures each attribute with divisor 1. Requires the target
    /// VertexArray to be bound by the caller (the attribute setup is recorded into it).
    static std::optional<InstanceBuffer> create(std::span<const float> data,
                                                GLsizei strideBytes,
                                                std::span<const InstanceAttribSpec> attribs,
                                                BufferAccessPattern accessPattern);

    [[nodiscard]]
    const BufferId& get_buffer_id() const;

    [[nodiscard]]
    GLsizei get_instance_count() const {
        return m_instanceCount;
    }

    void bind() const;

    static void unbind();

    /// Re-uploads the interleaved data (buffer orphaning) and recomputes the instance count.
    void update(std::span<const float> data, BufferAccessPattern accessPattern);

  private:
    InstanceBuffer(BufferId bufferId,
                   GLsizei strideBytes,
                   GLsizei instanceCount,
                   std::vector<InstanceAttribSpec> attribs);
};

} // namespace opengl

#endif // OPENGL_INSTANCEBUFFER_HPP
