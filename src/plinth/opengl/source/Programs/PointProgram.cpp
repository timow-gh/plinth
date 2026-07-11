#include "OpenGL/Programs/PointProgram.hpp"
#include "OpenGL/ErrorReporting.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"
#include <plinth/Assert.hpp>
#include <format>
#include <string>
#include <utility>

namespace opengl {

PointProgram::PointProgram(ProgramHandle program,
                           Uniform mvpLocation,
                           Uniform modelMatrixLocation,
                           Attribute vertexLocation,
                           Attribute colorLocation) noexcept
    : m_program{std::move(program)}
    , m_mvpLocation{mvpLocation}
    , m_modelMatrixLocation{modelMatrixLocation}
    , m_vertexLocation{vertexLocation}
    , m_colorLocation{colorLocation} {
    RENDERER_ASSERT(m_program.is_valid());
    RENDERER_ASSERT(mvpLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(modelMatrixLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(vertexLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(colorLocation.get_location().get_value() != -1);
}

void PointProgram::use() const {
    glUseProgram(m_program.get_value());
}

PointProgram make_point_program() {
    const std::string vertexShaderSource = point_color_vertex_shader_source();
    const std::string fragmentShaderSource = point_color_fragment_shader_source();

    ProgramCreationResult program = create_program(vertexShaderSource.c_str(), fragmentShaderSource.c_str());
    if (!program.has_value()) {
        report_error(std::format("Error category: '{}';Error code: '{}'; Error message: '{}'",
                                 program.error().category().name(),
                                 program.error().value(),
                                 program.error().message()));
        RENDERER_ASSERT(false);
        return {};
    }
    ProgramId programId = program->get_id();
    Uniform mvpLocation = make_uniform("u_MVP", programId);
    Uniform modelMatrixLocation = make_uniform("u_model", programId);
    Attribute vertexLocation = make_attribute("a_vertex", programId);
    Attribute colorLocation = make_attribute("a_color", programId);

    return PointProgram{std::move(*program), mvpLocation, modelMatrixLocation, vertexLocation, colorLocation};
}

} // namespace opengl
