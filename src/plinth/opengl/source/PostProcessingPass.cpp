#include "OpenGL/PostProcessingPass.hpp"

#include "OpenGL/ErrorReporting.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"

#include <array>
#include <format>
#include <utility>

namespace opengl {

namespace {

class ScopedFullscreenState {
  public:
    ScopedFullscreenState()
        : m_depthTest(glIsEnabled(GL_DEPTH_TEST))
        , m_cullFace(glIsEnabled(GL_CULL_FACE))
        , m_blend(glIsEnabled(GL_BLEND)) {
        glGetIntegerv(GL_VIEWPORT, m_viewport.data());
        glGetIntegerv(GL_CURRENT_PROGRAM, &m_program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_vertexArray);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &m_activeTexture);
        for (unsigned unit = 0; unit < 2; ++unit) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_textures[unit]);
        }
        glActiveTexture(static_cast<GLenum>(m_activeTexture));
    }

    ~ScopedFullscreenState() {
        glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
        glUseProgram(static_cast<GLuint>(m_program));
        glBindVertexArray(static_cast<GLuint>(m_vertexArray));
        for (unsigned unit = 0; unit < 2; ++unit) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_textures[unit]));
        }
        glActiveTexture(static_cast<GLenum>(m_activeTexture));
        m_depthTest != 0 ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
        m_cullFace != 0 ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        m_blend != 0 ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    }

    ScopedFullscreenState(const ScopedFullscreenState&) = delete;
    ScopedFullscreenState(ScopedFullscreenState&&) = delete;
    ScopedFullscreenState& operator=(const ScopedFullscreenState&) = delete;
    ScopedFullscreenState& operator=(ScopedFullscreenState&&) = delete;

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

PostProcessingPass::PostProcessingPass(ProgramHandle program,
                                       GLuint vertexArray,
                                       Uniform sceneColor,
                                       Uniform sceneDepth,
                                       Uniform reversedDepth,
                                       Uniform exposureStops,
                                       Uniform toneMapMode,
                                       Uniform visualizationMode,
                                       Uniform hdrDisplayMax,
                                       Uniform grayscale) noexcept
    : m_program(std::move(program))
    , m_vertexArray(vertexArray)
    , m_sceneColor(sceneColor)
    , m_sceneDepth(sceneDepth)
    , m_reversedDepth(reversedDepth)
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
    , m_reversedDepth(other.m_reversedDepth)
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
        m_reversedDepth = other.m_reversedDepth;
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
    Uniform reversedDepth = make_uniform("u_reversedDepth", pid);
    Uniform exposureStops = make_uniform("u_exposureStops", pid);
    Uniform toneMapMode = make_uniform("u_toneMapMode", pid);
    Uniform visualizationMode = make_uniform("u_visualizationMode", pid);
    Uniform hdrDisplayMax = make_uniform("u_hdrDisplayMax", pid);
    Uniform grayscale = make_uniform("u_grayscale", pid);

    if (sceneColor.get_location().get_value() == -1 || sceneDepth.get_location().get_value() == -1 ||
        reversedDepth.get_location().get_value() == -1 ||
        exposureStops.get_location().get_value() == -1 || toneMapMode.get_location().get_value() == -1 ||
        visualizationMode.get_location().get_value() == -1 || hdrDisplayMax.get_location().get_value() == -1 ||
        grayscale.get_location().get_value() == -1) {
        glDeleteVertexArrays(1, &vertexArray);
        report_error("PostProcessingPass::create: one or more uniforms not found");
        return std::nullopt;
    }

    return PostProcessingPass{std::move(*program),
                              vertexArray,
                              sceneColor,
                              sceneDepth,
                              reversedDepth,
                              exposureStops,
                              toneMapMode,
                              visualizationMode,
                              hdrDisplayMax,
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

void PostProcessingPass::set_reversed_depth(bool enabled) const {
    glUseProgram(m_program.get_value());
    glUniform1i(m_reversedDepth.get_location().get_value(), enabled ? 1 : 0);
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
    m_exposureStops = Uniform{};
    m_toneMapMode = Uniform{};
    m_visualizationMode = Uniform{};
    m_hdrDisplayMax = Uniform{};
    m_grayscale = Uniform{};
}

} // namespace opengl
