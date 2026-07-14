#ifndef OPENGL_PRESENTATIONPASS_HPP
#define OPENGL_PRESENTATIONPASS_HPP

#include <OpenGL/OpenGL.hpp>
#include <OpenGL/Programs/ProgramId.hpp>
#include <OpenGL/Uniform.hpp>
#include <OpenGL/opengl_export.h>
#include <optional>

namespace opengl {

class OPENGL_EXPORT PresentationPass {
  public:
    PresentationPass(const PresentationPass&) = delete;
    PresentationPass& operator=(const PresentationPass&) = delete;
    PresentationPass(PresentationPass&& other) noexcept;
    PresentationPass& operator=(PresentationPass&& other) noexcept;
    ~PresentationPass();

    [[nodiscard]]
    static std::optional<PresentationPass> create();
    [[nodiscard]]
    bool is_valid() const noexcept;
    void present(GLuint colorTexture, int width, int height) const;

  private:
    PresentationPass(ProgramHandle program, Uniform sceneColor, GLuint vertexArray) noexcept;
    void reset() noexcept;

    ProgramHandle m_program;
    Uniform m_sceneColor;
    GLuint m_vertexArray{0};
};

} // namespace opengl

#endif // OPENGL_PRESENTATIONPASS_HPP
