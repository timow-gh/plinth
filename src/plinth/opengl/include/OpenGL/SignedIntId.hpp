#ifndef OPENGL_SIGNEDINTID_HPP
#define OPENGL_SIGNEDINTID_HPP

#include "OpenGL/OpenGL.hpp"
#include "OpenGL/opengl_export.h"
#include <Renderer/Assert.hpp>

namespace opengl {

class OPENGL_EXPORT SignedIntId {
    GLint m_id{-1};

  public:
    constexpr SignedIntId() = default;
    constexpr explicit SignedIntId(GLint id) noexcept
        : m_id{id} {
        RENDERER_ASSERT(id != -1);
    }

    [[nodiscard]]
    constexpr GLint get_value() const noexcept {
        return m_id;
    }
    [[nodiscard]]
    constexpr GLuint get_as_unsigned() const noexcept {
        return static_cast<GLuint>(m_id);
    }
};

} // namespace opengl

#endif // OPENGL_SIGNEDINTID_HPP
