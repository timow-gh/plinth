#include "OpenGL/Programs/LineProgram.hpp"
#include "OpenGL/ErrorReporting.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"
#include "plinth/Assert.hpp"
#include <format>
#include <string>
#include <utility>

namespace opengl {

LineProgram::LineProgram(ProgramHandle program,
                         Uniform viewProjectionLocation,
                         Uniform modelMatrixLocation,
                         Uniform viewportSizeLocation,
                         Uniform lineWidthLocation,
                         Uniform dashSpaceLocation,
                         Uniform dashEnabledLocation,
                         Uniform dashSizeLocation,
                         Uniform gapSizeLocation,
                         Uniform dashPhaseLocation,
                         Uniform pickModeLocation,
                         Uniform pickColorLocation,
                         Attribute cornerLocation,
                         Attribute p0Location,
                         Attribute p1Location,
                         Attribute color0Location,
                         Attribute color1Location) noexcept
    : m_program{std::move(program)}
    , m_viewProjectionLocation{viewProjectionLocation}
    , m_modelMatrixLocation{modelMatrixLocation}
    , m_viewportSizeLocation{viewportSizeLocation}
    , m_lineWidthLocation{lineWidthLocation}
    , m_dashSpaceLocation{dashSpaceLocation}
    , m_dashEnabledLocation{dashEnabledLocation}
    , m_dashSizeLocation{dashSizeLocation}
    , m_gapSizeLocation{gapSizeLocation}
    , m_dashPhaseLocation{dashPhaseLocation}
    , m_pickModeLocation{pickModeLocation}
    , m_pickColorLocation{pickColorLocation}
    , m_cornerLocation{cornerLocation}
    , m_p0Location{p0Location}
    , m_p1Location{p1Location}
    , m_color0Location{color0Location}
    , m_color1Location{color1Location} {
    RENDERER_ASSERT(m_program.is_valid());
    RENDERER_ASSERT(viewProjectionLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(modelMatrixLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(viewportSizeLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(lineWidthLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(dashSpaceLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(dashEnabledLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(dashSizeLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(gapSizeLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(dashPhaseLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(pickModeLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(pickColorLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(cornerLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(p0Location.get_location().get_value() != -1);
    RENDERER_ASSERT(p1Location.get_location().get_value() != -1);
    RENDERER_ASSERT(color0Location.get_location().get_value() != -1);
    RENDERER_ASSERT(color1Location.get_location().get_value() != -1);
}

void LineProgram::use() const {
    glUseProgram(m_program.get_value());
}

opengl::LineProgram make_line_program() {
    const std::string vertexShaderSource = line_vertex_shader_source();
    const std::string fragmentShaderSource = line_fragment_shader_source();
    ProgramCreationResult program = create_program(vertexShaderSource.c_str(), fragmentShaderSource.c_str());
    if (!program) {
        report_error(std::format("Error category: '{}';Error code: '{}'; Error message: '{}'",
                                 program.error().category().name(),
                                 program.error().value(),
                                 program.error().message()));
        RENDERER_ASSERT(false);
        return {};
    }

    ProgramId id = program->get_id();
    RENDERER_ASSERT(id.get_value() != 0);
    Uniform viewProjectionLocation = make_uniform("u_viewProjection", id);
    Uniform modelMatrixLocation = make_uniform("u_model", id);
    Uniform viewportSizeLocation = make_uniform("u_viewportSize", id);
    Uniform lineWidthLocation = make_uniform("u_lineWidth", id);
    Uniform dashSpaceLocation = make_uniform("u_dashSpace", id);
    Uniform dashEnabledLocation = make_uniform("u_dashEnabled", id);
    Uniform dashSizeLocation = make_uniform("u_dashSize", id);
    Uniform gapSizeLocation = make_uniform("u_gapSize", id);
    Uniform dashPhaseLocation = make_uniform("u_dashPhase", id);
    Uniform pickModeLocation = make_uniform("u_pickMode", id);
    Uniform pickColorLocation = make_uniform("u_pickColor", id);
    Attribute cornerLocation = make_attribute("a_corner", id);
    Attribute p0Location = make_attribute("a_p0", id);
    Attribute p1Location = make_attribute("a_p1", id);
    Attribute color0Location = make_attribute("a_color0", id);
    Attribute color1Location = make_attribute("a_color1", id);
    return LineProgram{std::move(*program),
                       viewProjectionLocation,
                       modelMatrixLocation,
                       viewportSizeLocation,
                       lineWidthLocation,
                       dashSpaceLocation,
                       dashEnabledLocation,
                       dashSizeLocation,
                       gapSizeLocation,
                       dashPhaseLocation,
                       pickModeLocation,
                       pickColorLocation,
                       cornerLocation,
                       p0Location,
                       p1Location,
                       color0Location,
                       color1Location};
}

} // namespace opengl
