#ifndef OPENGL_VERTEXARRAY_HPP
#define OPENGL_VERTEXARRAY_HPP

#include "OpenGL/OpenGL.hpp"
#include "OpenGL/opengl_export.h"
#include <optional>

namespace opengl {

class OPENGL_EXPORT VertexArray {
    std::optional<GLuint> m_id;

  public:
    explicit VertexArray(GLuint id);
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    ~VertexArray();

    void reset() noexcept;

    [[nodiscard]]
    static std::optional<VertexArray> create();

    void bind() const;
    static void unbind();
};

} // namespace opengl

#endif // OPENGL_VERTEXARRAY_HPP
