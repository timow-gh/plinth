#include "OpenGL/PostProcessingPass.hpp"
#include "OpenGL/ErrorReporting.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"
#include <format>
#include <utility>

namespace opengl {

namespace {

class ScopedFullscreenState {
  public:
    ScopedFullscreenState() {
        glGetIntegerv(GL_VIEWPORT, m_viewport.data());
        glGetIntegerv(GL_CURRENT_PROGRAM, &m_program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_vertexArray);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &m_activeTexture);
        for (int unit = 0; unit < 2; ++unit) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_textures[unit]);
        }
        glActiveTexture(static_cast<GLenum>(m_activeTexture));
        m_depthTest = glIsEnabled(GL_DEPTH_TEST);
        m_cullFace = glIsEnabled(GL_CULL_FACE);
        m_blend = glIsEnabled(GL_BLEND);
    }

    ~ScopedFullscreenState() {
        glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
        glUseProgram(static_cast<GLuint>(m_program));
        glBindVertexArray(static_cast<GLuint>(m_vertexArray));
        for (int unit = 0; unit < 2; ++unit) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_textures[unit]));
        }
        glActiveTexture(static_cast<GLenum>(m_activeTexture));
        m_depthTest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
        m_cullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        m_blend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    }

  private:
    std::array<GLint, 4> m_viewport{};
    std::array<GLint, 2> m_textures{};
    GLint m_program{0};
    GLint m_vertexArray{0};
    GLint m_activeTexture{GL_TEXTURE0};
    GLboolean m_depthTest{GL_FALSE};
    GLboolean m_cullFace{GL_FALSE};
    GLboolean m_blend{GL_FALSE};
};

} // namespace

PostProcessingPass::PostProcessingPass(ProgramHandle program, GLuint vertexArray,
                                       Uniform sceneColor, Uniform sceneDepth,
                                       Uniform invProjection,
                                       Uniform fogEnabled, Uniform fogMode,
                                       Uniform fogStart, Uniform fogEnd, Uniform fogDensity, Uniform fogColor,
                                       Uniform exposureStops, Uniform toneMapMode,
                                       Uniform visualizationMode, Uniform hdrDisplayMax,
                                       Uniform grayscale) noexcept
    : m_program(std::move(program))
    , m_vertexArray(vertexArray)
    , m_sceneColor(sceneColor)
    , m_sceneDepth(sceneDepth)
    , m_invProjection(invProjection)
    , m_fogEnabled(fogEnabled)
    , m_fogMode(fogMode)
    , m_fogStart(fogStart)
    , m_fogEnd(fogEnd)
    , m_fogDensity(fogDensity)
    , m_fogColor(fogColor)
    , m_exposureStops(exposureStops)
    , m_toneMapMode(toneMapMode)
    , m_visualizationMode(visualizationMode)
    , m_hdrDisplayMax(hdrDisplayMax)
    , m_grayscale(grayscale) {
}

PostProcessingPass::PostProcessingPass(PostProcessingPass&& other) noexcept
    : m_program(std::move(other.m_program))
    , m_vertexArray(std::exchange(other.m_vertexArray, 0))
    , m_sceneColor(other.m_sceneColor)
    , m_sceneDepth(other.m_sceneDepth)
    , m_invProjection(other.m_invProjection)
    , m_fogEnabled(other.m_fogEnabled)
    , m_fogMode(other.m_fogMode)
    , m_fogStart(other.m_fogStart)
    , m_fogEnd(other.m_fogEnd)
    , m_fogDensity(other.m_fogDensity)
    , m_fogColor(other.m_fogColor)
    , m_exposureStops(other.m_exposureStops)
    , m_toneMapMode(other.m_toneMapMode)
    , m_visualizationMode(other.m_visualizationMode)
    , m_hdrDisplayMax(other.m_hdrDisplayMax)
    , m_grayscale(other.m_grayscale) {
}

PostProcessingPass& PostProcessingPass::operator=(PostProcessingPass&& other) noexcept {
    if (this != &other) {
        reset();
        m_program = std::move(other.m_program);
        m_vertexArray = std::exchange(other.m_vertexArray, 0);
        m_sceneColor = other.m_sceneColor;
        m_sceneDepth = other.m_sceneDepth;
        m_invProjection = other.m_invProjection;
        m_fogEnabled = other.m_fogEnabled;
        m_fogMode = other.m_fogMode;
        m_fogStart = other.m_fogStart;
        m_fogEnd = other.m_fogEnd;
        m_fogDensity = other.m_fogDensity;
        m_fogColor = other.m_fogColor;
        m_exposureStops = other.m_exposureStops;
        m_toneMapMode = other.m_toneMapMode;
        m_visualizationMode = other.m_visualizationMode;
        m_hdrDisplayMax = other.m_hdrDisplayMax;
        m_grayscale = other.m_grayscale;
    }
    return *this;
}

PostProcessingPass::~PostProcessingPass() {
    reset();
}

std::optional<PostProcessingPass> PostProcessingPass::create() {
    const std::string vertexSource = post_processing_vertex_shader_source();
    const std::string fragmentSource = post_processing_fragment_shader_source();
    ProgramCreationResult program = create_program(vertexSource.c_str(), fragmentSource.c_str());
    if (!program) {
        report_error(std::format("PostProcessingPass::create: {}", program.error().message()));
        return std::nullopt;
    }

    GLuint vertexArray{0};
    glGenVertexArrays(1, &vertexArray);
    if (vertexArray == 0) {
        report_error("PostProcessingPass::create: glGenVertexArrays failed");
        return std::nullopt;
    }

    const auto pid = program->get_id();

    Uniform sceneColor = make_uniform("u_sceneColor", pid);
    Uniform sceneDepth = make_uniform("u_sceneDepth", pid);
    Uniform invProjection = make_uniform("u_invProjection", pid);
    Uniform fogEnabled = make_uniform("u_fogEnabled", pid);
    Uniform fogMode = make_uniform("u_fogMode", pid);
    Uniform fogStart = make_uniform("u_fogStart", pid);
    Uniform fogEnd = make_uniform("u_fogEnd", pid);
    Uniform fogDensity = make_uniform("u_fogDensity", pid);
    Uniform fogColor = make_uniform("u_fogColor", pid);
    Uniform exposureStops = make_uniform("u_exposureStops", pid);
    Uniform toneMapMode = make_uniform("u_toneMapMode", pid);
    Uniform visualizationMode = make_uniform("u_visualizationMode", pid);
    Uniform hdrDisplayMax = make_uniform("u_hdrDisplayMax", pid);
    Uniform grayscale = make_uniform("u_grayscale", pid);

    if (sceneColor.get_location().get_value() == -1 ||
        sceneDepth.get_location().get_value() == -1 ||
        invProjection.get_location().get_value() == -1 ||
        fogEnabled.get_location().get_value() == -1 ||
        fogMode.get_location().get_value() == -1 ||
        fogStart.get_location().get_value() == -1 ||
        fogEnd.get_location().get_value() == -1 ||
        fogDensity.get_location().get_value() == -1 ||
        fogColor.get_location().get_value() == -1 ||
        exposureStops.get_location().get_value() == -1 ||
        toneMapMode.get_location().get_value() == -1 ||
        visualizationMode.get_location().get_value() == -1 ||
        hdrDisplayMax.get_location().get_value() == -1 ||
        grayscale.get_location().get_value() == -1) {
        glDeleteVertexArrays(1, &vertexArray);
        report_error("PostProcessingPass::create: one or more uniforms not found");
        return std::nullopt;
    }

    return PostProcessingPass{std::move(*program), vertexArray,
                              sceneColor, sceneDepth,
                              invProjection,
                              fogEnabled, fogMode, fogStart, fogEnd, fogDensity, fogColor,
                              exposureStops, toneMapMode,
                              visualizationMode, hdrDisplayMax,
                              grayscale};
}

bool PostProcessingPass::is_valid() const noexcept {
    return m_program.is_valid() && m_vertexArray != 0;
}

void PostProcessingPass::process(GLuint hdrColorTexture, GLuint depthTexture, int width, int height) const {
    ScopedFullscreenState state;
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glUseProgram(m_program.get_value());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrColorTexture);
    glUniform1i(m_sceneColor.get_location().get_value(), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(m_sceneDepth.get_location().get_value(), 1);
    glBindVertexArray(m_vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void PostProcessingPass::set_inv_projection(const float* data) const {
    glUseProgram(m_program.get_value());
    glUniformMatrix4fv(m_invProjection.get_location().get_value(), 1, GL_FALSE, data);
}

void PostProcessingPass::set_fog_enabled(bool enabled) const {
    glUseProgram(m_program.get_value());
    glUniform1i(m_fogEnabled.get_location().get_value(), enabled ? 1 : 0);
}

void PostProcessingPass::set_fog_mode(int mode) const {
    glUseProgram(m_program.get_value());
    glUniform1i(m_fogMode.get_location().get_value(), mode);
}

void PostProcessingPass::set_fog_start(float start) const {
    glUseProgram(m_program.get_value());
    glUniform1f(m_fogStart.get_location().get_value(), start);
}

void PostProcessingPass::set_fog_end(float end) const {
    glUseProgram(m_program.get_value());
    glUniform1f(m_fogEnd.get_location().get_value(), end);
}

void PostProcessingPass::set_fog_density(float density) const {
    glUseProgram(m_program.get_value());
    glUniform1f(m_fogDensity.get_location().get_value(), density);
}

void PostProcessingPass::set_fog_color(float r, float g, float b) const {
    glUseProgram(m_program.get_value());
    glUniform3f(m_fogColor.get_location().get_value(), r, g, b);
}

void PostProcessingPass::set_exposure_stops(float stops) const {
    glUseProgram(m_program.get_value());
    glUniform1f(m_exposureStops.get_location().get_value(), stops);
}

void PostProcessingPass::set_tone_map_mode(int mode) const {
    glUseProgram(m_program.get_value());
    glUniform1i(m_toneMapMode.get_location().get_value(), mode);
}

void PostProcessingPass::set_visualization_mode(int mode) const {
    glUseProgram(m_program.get_value());
    glUniform1i(m_visualizationMode.get_location().get_value(), mode);
}

void PostProcessingPass::set_hdr_display_max(float maxVal) const {
    glUseProgram(m_program.get_value());
    glUniform1f(m_hdrDisplayMax.get_location().get_value(), maxVal);
}

void PostProcessingPass::set_grayscale(bool enabled) const {
    glUseProgram(m_program.get_value());
    glUniform1i(m_grayscale.get_location().get_value(), enabled ? 1 : 0);
}

void PostProcessingPass::reset() noexcept {
    if (m_vertexArray != 0) {
        glDeleteVertexArrays(1, &m_vertexArray);
        m_vertexArray = 0;
    }
    m_program.reset();
    m_sceneColor = Uniform{};
    m_sceneDepth = Uniform{};
    m_invProjection = Uniform{};
    m_fogEnabled = Uniform{};
    m_fogMode = Uniform{};
    m_fogStart = Uniform{};
    m_fogEnd = Uniform{};
    m_fogDensity = Uniform{};
    m_fogColor = Uniform{};
    m_exposureStops = Uniform{};
    m_toneMapMode = Uniform{};
    m_visualizationMode = Uniform{};
    m_hdrDisplayMax = Uniform{};
    m_grayscale = Uniform{};
}

} // namespace opengl
