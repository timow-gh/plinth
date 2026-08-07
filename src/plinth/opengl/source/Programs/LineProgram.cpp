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
                         Uniform dashPhaseLocation,
                         Uniform pickModeLocation,
                         Uniform pickColorLocation,
                         Uniform capStyleLocation,
                         Uniform joinStyleLocation,
                         Uniform dashPatternCountLocation,
                         Uniform dashPatternLocation,
                         Attribute cornerLocation,
                         Attribute p0Location,
                         Attribute p1Location,
                         Attribute color0Location,
                         Attribute color1Location,
                         Attribute pPrevLocation,
                         Attribute pNextLocation) noexcept
    : m_program{std::move(program)}
    , m_viewProjectionLocation{viewProjectionLocation}
    , m_modelMatrixLocation{modelMatrixLocation}
    , m_viewportSizeLocation{viewportSizeLocation}
    , m_lineWidthLocation{lineWidthLocation}
    , m_dashSpaceLocation{dashSpaceLocation}
    , m_dashPhaseLocation{dashPhaseLocation}
    , m_pickModeLocation{pickModeLocation}
    , m_pickColorLocation{pickColorLocation}
    , m_capStyleLocation{capStyleLocation}
    , m_joinStyleLocation{joinStyleLocation}
    , m_dashPatternCountLocation{dashPatternCountLocation}
    , m_dashPatternLocation{dashPatternLocation}
    , m_cornerLocation{cornerLocation}
    , m_p0Location{p0Location}
    , m_p1Location{p1Location}
    , m_color0Location{color0Location}
    , m_color1Location{color1Location}
    , m_pPrevLocation{pPrevLocation}
    , m_pNextLocation{pNextLocation} {
    RENDERER_ASSERT(m_program.is_valid());
    RENDERER_ASSERT(viewProjectionLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(modelMatrixLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(viewportSizeLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(lineWidthLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(dashSpaceLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(dashPhaseLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(pickModeLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(pickColorLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(capStyleLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(joinStyleLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(dashPatternCountLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(dashPatternLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(cornerLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(p0Location.get_location().get_value() != -1);
    RENDERER_ASSERT(p1Location.get_location().get_value() != -1);
    RENDERER_ASSERT(color0Location.get_location().get_value() != -1);
    RENDERER_ASSERT(color1Location.get_location().get_value() != -1);
    RENDERER_ASSERT(pPrevLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(pNextLocation.get_location().get_value() != -1);
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
    Uniform dashPhaseLocation = make_uniform("u_dashPhase", id);
    Uniform pickModeLocation = make_uniform("u_pickMode", id);
    Uniform pickColorLocation = make_uniform("u_pickColor", id);
    Uniform capStyleLocation = make_uniform("u_capStyle", id);
    Uniform joinStyleLocation = make_uniform("u_joinStyle", id);
    Uniform dashPatternCountLocation = make_uniform("u_dashPatternCount", id);
    Uniform dashPatternLocation = make_uniform("u_dashPattern", id);
    Attribute cornerLocation = make_attribute("a_corner", id);
    Attribute p0Location = make_attribute("a_p0", id);
    Attribute p1Location = make_attribute("a_p1", id);
    Attribute color0Location = make_attribute("a_color0", id);
    Attribute color1Location = make_attribute("a_color1", id);
    Attribute pPrevLocation = make_attribute("a_pPrev", id);
    Attribute pNextLocation = make_attribute("a_pNext", id);
    return LineProgram{std::move(*program),
                       viewProjectionLocation,
                       modelMatrixLocation,
                       viewportSizeLocation,
                       lineWidthLocation,
                       dashSpaceLocation,
                       dashPhaseLocation,
                       pickModeLocation,
                       pickColorLocation,
                       capStyleLocation,
                       joinStyleLocation,
                       dashPatternCountLocation,
                       dashPatternLocation,
                       cornerLocation,
                       p0Location,
                       p1Location,
                       color0Location,
                       color1Location,
                       pPrevLocation,
                       pNextLocation};
}

} // namespace opengl
