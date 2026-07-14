#include "OpenGL/PresentationPass.hpp"
#include "OpenGL/ErrorReporting.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"
#include <format>
#include <utility>

namespace opengl {

PresentationPass::PresentationPass(ProgramHandle program, Uniform sceneColor, GLuint vertexArray) noexcept
    : m_program(std::move(program))
    , m_sceneColor(sceneColor)
    , m_vertexArray(vertexArray) {
}

PresentationPass::PresentationPass(PresentationPass&& other) noexcept
    : m_program(std::move(other.m_program))
    , m_sceneColor(other.m_sceneColor)
    , m_vertexArray(std::exchange(other.m_vertexArray, 0)) {
}

PresentationPass& PresentationPass::operator=(PresentationPass&& other) noexcept {
    if (this != &other) {
        reset();
        m_program = std::move(other.m_program);
        m_sceneColor = other.m_sceneColor;
        m_vertexArray = std::exchange(other.m_vertexArray, 0);
    }
    return *this;
}

PresentationPass::~PresentationPass() {
    reset();
}

std::optional<PresentationPass> PresentationPass::create() {
    const std::string vertexSource = presentation_vertex_shader_source();
    const std::string fragmentSource = presentation_fragment_shader_source();
    ProgramCreationResult program = create_program(vertexSource.c_str(), fragmentSource.c_str());
    if (!program) {
        report_error(std::format("PresentationPass::create: {}", program.error().message()));
        return std::nullopt;
    }

    GLuint vertexArray{0};
    glGenVertexArrays(1, &vertexArray);
    if (vertexArray == 0) {
        report_error("PresentationPass::create: glGenVertexArrays failed");
        return std::nullopt;
    }

    Uniform sceneColor = make_uniform("u_sceneColor", program->get_id());
    if (sceneColor.get_location().get_value() == -1) {
        glDeleteVertexArrays(1, &vertexArray);
        report_error("PresentationPass::create: u_sceneColor uniform not found");
        return std::nullopt;
    }

    return PresentationPass{std::move(*program), sceneColor, vertexArray};
}

bool PresentationPass::is_valid() const noexcept {
    return m_program.is_valid() && m_vertexArray != 0 && m_sceneColor.get_location().get_value() != -1;
}

void PresentationPass::present(GLuint colorTexture, int width, int height) const {
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glUseProgram(m_program.get_value());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glUniform1i(m_sceneColor.get_location().get_value(), 0);
    glBindVertexArray(m_vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void PresentationPass::reset() noexcept {
    if (m_vertexArray != 0) {
        glDeleteVertexArrays(1, &m_vertexArray);
        m_vertexArray = 0;
    }
    m_program.reset();
    m_sceneColor = Uniform{};
}

} // namespace opengl
