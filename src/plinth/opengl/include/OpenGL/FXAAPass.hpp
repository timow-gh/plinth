#ifndef OPENGL_FXAAPASS_HPP
#define OPENGL_FXAAPASS_HPP

#include <OpenGL/OpenGL.hpp>
#include <OpenGL/Programs/ProgramId.hpp>
#include <OpenGL/Uniform.hpp>
#include <OpenGL/opengl_export.h>
#include <optional>

namespace opengl {

class OPENGL_EXPORT FXAAPass {
  public:
    FXAAPass(const FXAAPass&) = delete;
    FXAAPass& operator=(const FXAAPass&) = delete;
    FXAAPass(FXAAPass&&) noexcept;
    FXAAPass& operator=(FXAAPass&&) noexcept;
    ~FXAAPass();

    [[nodiscard]]
    static std::optional<FXAAPass> create();

    [[nodiscard]]
    bool is_valid() const noexcept;

    void process(GLuint inputTexture, int width, int height) const;

    void set_enabled(bool enabled) const;
    void set_edge_threshold(float threshold) const;
    void set_edge_threshold_min(float threshold) const;
    void set_subpixel_amount(float amount) const;

  private:
    FXAAPass(ProgramHandle program, GLuint vertexArray,
             Uniform inputTexture, Uniform invTextureSize,
             Uniform enabled, Uniform edgeThreshold,
             Uniform edgeThresholdMin, Uniform subpixelAmount) noexcept;
    void reset() noexcept;

    ProgramHandle m_program;
    GLuint m_vertexArray{0};
    Uniform m_inputTexture;
    Uniform m_invTextureSize;
    Uniform m_enabled;
    Uniform m_edgeThreshold;
    Uniform m_edgeThresholdMin;
    Uniform m_subpixelAmount;
};

} // namespace opengl

#endif
