#ifndef OPENGL_POSTPROCESSINGPASS_HPP
#define OPENGL_POSTPROCESSINGPASS_HPP

#include <OpenGL/OpenGL.hpp>
#include <OpenGL/Programs/ProgramId.hpp>
#include <OpenGL/Uniform.hpp>
#include <OpenGL/opengl_export.h>
#include <optional>

namespace opengl {

class OPENGL_EXPORT PostProcessingPass {
  public:
    PostProcessingPass(const PostProcessingPass&) = delete;
    PostProcessingPass& operator=(const PostProcessingPass&) = delete;
    PostProcessingPass(PostProcessingPass&&) noexcept;
    PostProcessingPass& operator=(PostProcessingPass&&) noexcept;
    ~PostProcessingPass();

    [[nodiscard]]
    static std::optional<PostProcessingPass> create();

    [[nodiscard]]
    bool is_valid() const noexcept;

    void process(GLuint hdrColorTexture, GLuint depthTexture, int width, int height) const;

    void set_inv_projection(const float* data) const;
    void set_fog_enabled(bool enabled) const;
    void set_fog_mode(int mode) const;
    void set_fog_start(float start) const;
    void set_fog_end(float end) const;
    void set_fog_density(float density) const;
    void set_fog_color(float r, float g, float b) const;
    void set_exposure_stops(float stops) const;
    void set_tone_map_mode(int mode) const;
    void set_visualization_mode(int mode) const;
    void set_hdr_display_max(float maxVal) const;
    void set_grayscale(bool enabled) const;

  private:
    PostProcessingPass(ProgramHandle program, GLuint vertexArray,
                       Uniform sceneColor, Uniform sceneDepth,
                       Uniform invProjection,
                       Uniform fogEnabled, Uniform fogMode,
                       Uniform fogStart, Uniform fogEnd, Uniform fogDensity, Uniform fogColor,
                       Uniform exposureStops, Uniform toneMapMode,
                       Uniform visualizationMode, Uniform hdrDisplayMax,
                       Uniform grayscale) noexcept;
    void reset() noexcept;

    ProgramHandle m_program;
    GLuint m_vertexArray{0};

    Uniform m_sceneColor;
    Uniform m_sceneDepth;
    Uniform m_invProjection;
    Uniform m_fogEnabled;
    Uniform m_fogMode;
    Uniform m_fogStart;
    Uniform m_fogEnd;
    Uniform m_fogDensity;
    Uniform m_fogColor;
    Uniform m_exposureStops;
    Uniform m_toneMapMode;
    Uniform m_visualizationMode;
    Uniform m_hdrDisplayMax;
    Uniform m_grayscale;
};

} // namespace opengl

#endif
