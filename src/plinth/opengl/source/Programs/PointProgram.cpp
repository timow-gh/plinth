#include "OpenGL/Programs/PointProgram.hpp"

#include "OpenGL/ErrorReporting.hpp"
#include "OpenGL/FrameUniforms.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"
#include "plinth/Assert.hpp"

#include <format>
#include <string>
#include <utility>

namespace opengl {

PointProgram::PointProgram(ProgramHandle program,
                           Uniform modelMatrixLocation,
                           Attribute vertexLocation,
                           Attribute colorLocation,
                           Uniform pickModeLocation,
                           Uniform pickColorLocation,
                           Uniform depthBiasLocation) noexcept
    : m_program{std::move(program)}
    , m_modelMatrixLocation{modelMatrixLocation}
    , m_vertexLocation{vertexLocation}
    , m_colorLocation{colorLocation}
    , m_pickModeLocation{pickModeLocation}
    , m_pickColorLocation{pickColorLocation}
    , m_depthBiasLocation{depthBiasLocation} {
    RENDERER_ASSERT(m_program.is_valid());
    RENDERER_ASSERT(modelMatrixLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(vertexLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(colorLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(pickModeLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(pickColorLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(depthBiasLocation.get_location().get_value() != -1);
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
    Uniform modelMatrixLocation = make_uniform("u_model", programId);
    Attribute vertexLocation = make_attribute("a_vertex", programId);
    Attribute colorLocation = make_attribute("a_color", programId);
    Uniform pickModeLocation = make_uniform("u_pickMode", programId);
    Uniform pickColorLocation = make_uniform("u_pickColor", programId);
    Uniform depthBiasLocation = make_uniform("u_depthBias", programId);

    bind_uniform_block(programId, "FrameBlock", kFrameUniformBinding);

    return PointProgram{std::move(*program),
                        modelMatrixLocation,
                        vertexLocation,
                        colorLocation,
                        pickModeLocation,
                        pickColorLocation,
                        depthBiasLocation};
}

} // namespace opengl
