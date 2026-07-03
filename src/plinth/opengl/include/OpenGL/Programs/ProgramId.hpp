#ifndef OPENGL_PROGRAMID_HPP
#define OPENGL_PROGRAMID_HPP

#include "OpenGL/OpenGL.hpp"
#include "OpenGL/UnsignedIntId.hpp"
#include "OpenGL/opengl_export.h"
#include <optional>

namespace opengl {

class OPENGL_EXPORT ProgramId : public UnsignedIntId {
  public:
    using UnsignedIntId::UnsignedIntId;
};

class OPENGL_EXPORT ProgramHandle {
    std::optional<ProgramId> m_id;

  public:
    ProgramHandle() noexcept = default;
    explicit ProgramHandle(ProgramId id) noexcept;
    explicit ProgramHandle(GLuint id) noexcept;

    ProgramHandle(const ProgramHandle&) = delete;
    ProgramHandle& operator=(const ProgramHandle&) = delete;
    ProgramHandle(ProgramHandle&& other) noexcept;
    ProgramHandle& operator=(ProgramHandle&& other) noexcept;
    ~ProgramHandle();

    void reset() noexcept;

    [[nodiscard]]
    bool is_valid() const noexcept {
        return m_id.has_value();
    }
    [[nodiscard]]
    ProgramId get_id() const;
    [[nodiscard]]
    GLuint get_value() const;
};

} // namespace opengl

#endif // OPENGL_PROGRAMID_HPP
