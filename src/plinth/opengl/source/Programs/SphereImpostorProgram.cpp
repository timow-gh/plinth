#include "OpenGL/Programs/SphereImpostorProgram.hpp"

#include "OpenGL/ErrorReporting.hpp"
#include "OpenGL/FrameUniforms.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"
#include "plinth/Assert.hpp"

#include <format>
#include <string>
#include <utility>

namespace opengl {

SphereImpostorProgram::SphereImpostorProgram(ProgramHandle program,
                                             Uniform modelMatrixLocation,
                                             Uniform inverseModelViewMatrixLocation,
                                             Uniform normalMatrixLocation,
                                             Uniform sizeSpaceLocation,
                                             Uniform pickModeLocation,
                                             Uniform pickColorLocation,
                                             Attribute sphereLocation,
                                             Attribute colorLocation) noexcept
    : m_program{std::move(program)}
    , m_modelMatrixLocation{modelMatrixLocation}
    , m_inverseModelViewMatrixLocation{inverseModelViewMatrixLocation}
    , m_normalMatrixLocation{normalMatrixLocation}
    , m_sizeSpaceLocation{sizeSpaceLocation}
    , m_pickModeLocation{pickModeLocation}
    , m_pickColorLocation{pickColorLocation}
    , m_sphereLocation{sphereLocation}
    , m_colorLocation{colorLocation} {
    RENDERER_ASSERT(m_program.is_valid());
    RENDERER_ASSERT(modelMatrixLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(inverseModelViewMatrixLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(normalMatrixLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(sizeSpaceLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(pickModeLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(pickColorLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(sphereLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(colorLocation.get_location().get_value() != -1);
}

void SphereImpostorProgram::use() const {
    glUseProgram(m_program.get_value());
}

SphereImpostorProgram make_sphere_impostor_program() {
    const std::string vertexShaderSource = sphere_impostor_vertex_shader_source();
    const std::string fragmentShaderSource = sphere_impostor_fragment_shader_source();
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

    Uniform modelMatrixLocation = make_uniform("u_model", id);
    Uniform inverseModelViewMatrixLocation = make_uniform("u_inverseModelView", id);
    Uniform normalMatrixLocation = make_uniform("u_normalMatrix", id);
    Uniform sizeSpaceLocation = make_uniform("u_sizeSpace", id);
    Uniform pickModeLocation = make_uniform("u_pickMode", id);
    Uniform pickColorLocation = make_uniform("u_pickColor", id);

    Attribute sphereLocation = make_attribute("a_sphere", id);
    Attribute colorLocation = make_attribute("a_color", id);

    bind_uniform_block(id, "FrameBlock", kFrameUniformBinding);

    return SphereImpostorProgram{std::move(*program),
                                 modelMatrixLocation,
                                 inverseModelViewMatrixLocation,
                                 normalMatrixLocation,
                                 sizeSpaceLocation,
                                 pickModeLocation,
                                 pickColorLocation,
                                 sphereLocation,
                                 colorLocation};
}

} // namespace opengl
