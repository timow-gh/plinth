#ifndef OPENGL_UNIFORMBUFFER_HPP
#define OPENGL_UNIFORMBUFFER_HPP

#include "OpenGL/BufferId.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/opengl_export.h"

#include <optional>
#include <span>

namespace opengl {

/** @brief A GL_UNIFORM_BUFFER bound to an indexed buffer binding point.
 *
 * Unlike VertexBuffer/InstanceBuffer (GL_ARRAY_BUFFER feeding vertex attributes), a UniformBuffer
 * feeds a std140 uniform block. It is created against a fixed binding point so every program whose
 * uniform block is linked to that binding reads the same, once-per-frame data.
 */
class OPENGL_EXPORT UniformBuffer {
    std::optional<BufferId> m_bufferId;
    GLuint m_bindingPoint{0};

  public:
    UniformBuffer() = default;
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;
    UniformBuffer(UniformBuffer&& other) noexcept;
    UniformBuffer& operator=(UniformBuffer&& other) noexcept;
    ~UniformBuffer();

    void reset() noexcept;

    [[nodiscard]] bool is_valid() const noexcept { return m_bufferId.has_value(); }

    /// Creates the buffer, uploads the initial data, and binds it to \p bindingPoint.
    [[nodiscard]] static std::optional<UniformBuffer> create(std::span<const float> data, GLuint bindingPoint);

    [[nodiscard]] const BufferId& get_buffer_id() const;

    /// Binds the buffer to its binding point via glBindBufferBase.
    void bind() const;

    /// Re-uploads the data (orphaning first) and keeps the buffer bound to its binding point.
    void update(std::span<const float> data);

  private:
    UniformBuffer(BufferId bufferId, GLuint bindingPoint);
};

} // namespace opengl

#endif // OPENGL_UNIFORMBUFFER_HPP
