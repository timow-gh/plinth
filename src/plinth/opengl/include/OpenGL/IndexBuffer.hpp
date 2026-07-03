#ifndef OPENGL_INDEXBUFFER_HPP
#define OPENGL_INDEXBUFFER_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/BufferId.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/opengl_export.h"
#include <plinth/Warnings.hpp>
#include <cstdint>
#include <optional>
#include <span>

RENDERER_DISABLE_ALL_WARNINGS

namespace opengl {

/** @brief An opengl buffer object for indices.
 *
 * @details Represents the data on the GPU.
 */
class OPENGL_EXPORT IndexBuffer {
    std::optional<BufferId> m_id;
    GLsizei m_indexCount{0};

  public:
    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;
    IndexBuffer(IndexBuffer&& other) noexcept;
    IndexBuffer& operator=(IndexBuffer&& other) noexcept;
    ~IndexBuffer();

    void reset() noexcept;

    static std::optional<IndexBuffer> create(std::span<const std::uint32_t> indices, BufferAccessPattern accessPattern);

    [[nodiscard]]
    const BufferId& get_buffer_id() const;

    [[nodiscard]]
    GLsizei get_index_count() const {
        return m_indexCount;
    }
    void set_index_count(GLsizei indexCount) { m_indexCount = indexCount; }

    void bind() const;
    static void unbind();

    void update_indices_buffer(std::span<const std::uint32_t> indices, BufferAccessPattern accessPattern);

  private:
    IndexBuffer(BufferId id, GLsizei indexCount);
};

} // namespace opengl

RENDERER_ENABLE_ALL_WARNINGS

#endif // OPENGL_INDEXBUFFER_HPP
