#include "OpenGL/FXAAPass.hpp"
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
    ScopedFullscreenState() {
        glGetIntegerv(GL_VIEWPORT, m_viewport.data());
        glGetIntegerv(GL_CURRENT_PROGRAM, &m_program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_vertexArray);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &m_activeTexture);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_texture);
        m_depthTest = glIsEnabled(GL_DEPTH_TEST);
        m_cullFace = glIsEnabled(GL_CULL_FACE);
        m_blend = glIsEnabled(GL_BLEND);
    }

    ~ScopedFullscreenState() {
        glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
        glUseProgram(static_cast<GLuint>(m_program));
        glBindVertexArray(static_cast<GLuint>(m_vertexArray));
        glActiveTexture(static_cast<GLenum>(m_activeTexture));
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_texture));
        m_depthTest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
        m_cullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        m_blend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    }

  private:
    std::array<GLint, 4> m_viewport{};
    GLint m_program{0};
    GLint m_vertexArray{0};
    GLint m_activeTexture{GL_TEXTURE0};
    GLint m_texture{0};
    GLboolean m_depthTest{GL_FALSE};
    GLboolean m_cullFace{GL_FALSE};
    GLboolean m_blend{GL_FALSE};
};

} // namespace

FXAAPass::FXAAPass(ProgramHandle program, GLuint vertexArray,
                   Uniform inputTexture, Uniform invTextureSize,
                   Uniform enabled, Uniform edgeThreshold,
                   Uniform edgeThresholdMin, Uniform subpixelAmount) noexcept
    : m_program(std::move(program))
    , m_vertexArray(vertexArray)
    , m_inputTexture(inputTexture)
    , m_invTextureSize(invTextureSize)
    , m_enabled(enabled)
    , m_edgeThreshold(edgeThreshold)
    , m_edgeThresholdMin(edgeThresholdMin)
    , m_subpixelAmount(subpixelAmount) {
}

FXAAPass::FXAAPass(FXAAPass&& other) noexcept
    : m_program(std::move(other.m_program))
    , m_vertexArray(std::exchange(other.m_vertexArray, 0))
    , m_inputTexture(other.m_inputTexture)
    , m_invTextureSize(other.m_invTextureSize)
    , m_enabled(other.m_enabled)
    , m_edgeThreshold(other.m_edgeThreshold)
    , m_edgeThresholdMin(other.m_edgeThresholdMin)
    , m_subpixelAmount(other.m_subpixelAmount) {
}

FXAAPass& FXAAPass::operator=(FXAAPass&& other) noexcept {
    if (this != &other) {
        reset();
        m_program = std::move(other.m_program);
        m_vertexArray = std::exchange(other.m_vertexArray, 0);
        m_inputTexture = other.m_inputTexture;
        m_invTextureSize = other.m_invTextureSize;
        m_enabled = other.m_enabled;
        m_edgeThreshold = other.m_edgeThreshold;
        m_edgeThresholdMin = other.m_edgeThresholdMin;
        m_subpixelAmount = other.m_subpixelAmount;
    }
    return *this;
}

FXAAPass::~FXAAPass() {
    reset();
}

std::optional<FXAAPass> FXAAPass::create() {
    const std::string vertexSource = fxaa_vertex_shader_source();
    const std::string fragmentSource = fxaa_fragment_shader_source();
    ProgramCreationResult program = create_program(vertexSource.c_str(), fragmentSource.c_str());
    if (!program) {
        report_error(std::format("FXAAPass::create: {}", program.error().message()));
        return std::nullopt;
    }

    GLuint vertexArray{0};
    glGenVertexArrays(1, &vertexArray);
    if (vertexArray == 0) {
        report_error("FXAAPass::create: glGenVertexArrays failed");
        return std::nullopt;
    }

    const auto pid = program->get_id();

    Uniform inputTexture = make_uniform("u_inputTexture", pid);
    Uniform invTextureSize = make_uniform("u_invTextureSize", pid);
    Uniform enabled = make_uniform("u_fxaaEnabled", pid);
    Uniform edgeThreshold = make_uniform("u_edgeThreshold", pid);
    Uniform edgeThresholdMin = make_uniform("u_edgeThresholdMin", pid);
    Uniform subpixelAmount = make_uniform("u_subpixelAmount", pid);

    if (inputTexture.get_location().get_value() == -1 ||
        invTextureSize.get_location().get_value() == -1 ||
        enabled.get_location().get_value() == -1 ||
        edgeThreshold.get_location().get_value() == -1 ||
        edgeThresholdMin.get_location().get_value() == -1 ||
        subpixelAmount.get_location().get_value() == -1) {
        glDeleteVertexArrays(1, &vertexArray);
        report_error("FXAAPass::create: one or more uniforms not found");
        return std::nullopt;
    }

    return FXAAPass{std::move(*program), vertexArray,
                    inputTexture, invTextureSize,
                    enabled, edgeThreshold, edgeThresholdMin, subpixelAmount};
}

bool FXAAPass::is_valid() const noexcept {
    return m_program.is_valid() && m_vertexArray != 0;
}

void FXAAPass::process(GLuint inputTexture, int width, int height) const {
    ScopedFullscreenState state;
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glUseProgram(m_program.get_value());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glUniform1i(m_inputTexture.get_location().get_value(), 0);

    const std::array<float, 2> invSize{1.0F / static_cast<float>(width), 1.0F / static_cast<float>(height)};
    glUniform2fv(m_invTextureSize.get_location().get_value(), 1, invSize.data());

    glBindVertexArray(m_vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void FXAAPass::set_enabled(bool enabled) const {
    glUseProgram(m_program.get_value());
    glUniform1i(m_enabled.get_location().get_value(), enabled ? 1 : 0);
}

void FXAAPass::set_edge_threshold(float threshold) const {
    glUseProgram(m_program.get_value());
    glUniform1f(m_edgeThreshold.get_location().get_value(), threshold);
}

void FXAAPass::set_edge_threshold_min(float threshold) const {
    glUseProgram(m_program.get_value());
    glUniform1f(m_edgeThresholdMin.get_location().get_value(), threshold);
}

void FXAAPass::set_subpixel_amount(float amount) const {
    glUseProgram(m_program.get_value());
    glUniform1f(m_subpixelAmount.get_location().get_value(), amount);
}

void FXAAPass::reset() noexcept {
    if (m_vertexArray != 0) {
        glDeleteVertexArrays(1, &m_vertexArray);
        m_vertexArray = 0;
    }
    m_program.reset();
    m_inputTexture = Uniform{};
    m_invTextureSize = Uniform{};
    m_enabled = Uniform{};
    m_edgeThreshold = Uniform{};
    m_edgeThresholdMin = Uniform{};
    m_subpixelAmount = Uniform{};
}

} // namespace opengl
