#ifndef OPENGL_UNSIGNEDINTID_HPP
#define OPENGL_UNSIGNEDINTID_HPP

#include "OpenGL/OpenGL.hpp"
#include "OpenGL/opengl_export.h"
#include <plinth/Assert.hpp>

namespace opengl {

class OPENGL_EXPORT UnsignedIntId {
    GLuint m_id{0};

  public:
    constexpr UnsignedIntId() noexcept = default;
    constexpr explicit UnsignedIntId(GLuint id) noexcept
        : m_id{id} {
        RENDERER_ASSERT(id != 0);
    }
    UnsignedIntId(const UnsignedIntId&) = default;
    UnsignedIntId& operator=(const UnsignedIntId&) = default;
    UnsignedIntId(UnsignedIntId&&) noexcept = default;
    UnsignedIntId& operator=(UnsignedIntId&&) noexcept = default;
    ~UnsignedIntId() = default;

    // Some OpenGL functions require a lvalue reference to GLuint
    [[nodiscard]]
    constexpr const GLuint& get_value() const noexcept {
        return m_id;
    }
    [[nodiscard]]
    constexpr GLuint& get_value() noexcept {
        return m_id;
    }
};

} // namespace opengl

#endif // OPENGL_UNSIGNEDINTID_HPP
