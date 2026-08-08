#include "OpenGL/Programs/SphereImpostorProgram.hpp"

#include "OpenGL/ErrorReporting.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"
#include "plinth/Assert.hpp"

#include <format>
#include <string>
#include <utility>

namespace opengl {

SphereImpostorProgram::SphereImpostorProgram(ProgramHandle program,
                                             Uniform modelMatrixLocation,
                                             Uniform viewMatrixLocation,
                                             Uniform projectionMatrixLocation,
                                             Uniform inverseModelViewMatrixLocation,
                                             Uniform normalMatrixLocation,
                                             Uniform invProjectionLocation,
                                             Uniform viewportSizeLocation,
                                             Uniform zeroToOneDepthLocation,
                                             Uniform lightPosLocation,
                                             Uniform lightColorLocation,
                                             Uniform fillLightDirectionLocation,
                                             Uniform fillLightColorLocation,
                                             Uniform ambientColorLocation,
                                             Uniform shininessLocation,
                                             Uniform lightAttenuationLocation,
                                             Uniform materialAmbientLocation,
                                             Uniform materialDiffuseLocation,
                                             Uniform materialSpecularLocation,
                                             Uniform pickModeLocation,
                                             Uniform pickColorLocation,
                                             Attribute sphereLocation,
                                             Attribute colorLocation) noexcept
    : m_program{std::move(program)}
    , m_modelMatrixLocation{modelMatrixLocation}
    , m_viewMatrixLocation{viewMatrixLocation}
    , m_projectionMatrixLocation{projectionMatrixLocation}
    , m_inverseModelViewMatrixLocation{inverseModelViewMatrixLocation}
    , m_normalMatrixLocation{normalMatrixLocation}
    , m_invProjectionLocation{invProjectionLocation}
    , m_viewportSizeLocation{viewportSizeLocation}
    , m_zeroToOneDepthLocation{zeroToOneDepthLocation}
    , m_lightPosLocation{lightPosLocation}
    , m_lightColorLocation{lightColorLocation}
    , m_fillLightDirectionLocation{fillLightDirectionLocation}
    , m_fillLightColorLocation{fillLightColorLocation}
    , m_ambientColorLocation{ambientColorLocation}
    , m_shininessLocation{shininessLocation}
    , m_lightAttenuationLocation{lightAttenuationLocation}
    , m_materialAmbientLocation{materialAmbientLocation}
    , m_materialDiffuseLocation{materialDiffuseLocation}
    , m_materialSpecularLocation{materialSpecularLocation}
    , m_pickModeLocation{pickModeLocation}
    , m_pickColorLocation{pickColorLocation}
    , m_sphereLocation{sphereLocation}
    , m_colorLocation{colorLocation} {
    RENDERER_ASSERT(m_program.is_valid());
    RENDERER_ASSERT(modelMatrixLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(viewMatrixLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(projectionMatrixLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(inverseModelViewMatrixLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(normalMatrixLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(invProjectionLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(viewportSizeLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(zeroToOneDepthLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(lightPosLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(lightColorLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(fillLightDirectionLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(fillLightColorLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(ambientColorLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(shininessLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(lightAttenuationLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(materialAmbientLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(materialDiffuseLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(materialSpecularLocation.get_location().get_value() != -1);
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
    Uniform viewMatrixLocation = make_uniform("u_view", id);
    Uniform projectionMatrixLocation = make_uniform("u_projection", id);
    Uniform inverseModelViewMatrixLocation = make_uniform("u_inverseModelView", id);
    Uniform normalMatrixLocation = make_uniform("u_normalMatrix", id);
    Uniform invProjectionLocation = make_uniform("u_invProjection", id);
    Uniform viewportSizeLocation = make_uniform("u_viewportSize", id);
    Uniform zeroToOneDepthLocation = make_uniform("u_zeroToOneDepth", id);
    Uniform lightPosLocation = make_uniform("u_lightPos", id);
    Uniform lightColorLocation = make_uniform("u_lightColor", id);
    Uniform fillLightDirectionLocation = make_uniform("u_fillLightDirection", id);
    Uniform fillLightColorLocation = make_uniform("u_fillLightColor", id);
    Uniform ambientColorLocation = make_uniform("u_ambientColor", id);
    Uniform shininessLocation = make_uniform("u_shininess", id);
    Uniform lightAttenuationLocation = make_uniform("u_lightAttenuation", id);
    Uniform materialAmbientLocation = make_uniform("u_materialAmbient", id);
    Uniform materialDiffuseLocation = make_uniform("u_materialDiffuse", id);
    Uniform materialSpecularLocation = make_uniform("u_materialSpecular", id);
    Uniform pickModeLocation = make_uniform("u_pickMode", id);
    Uniform pickColorLocation = make_uniform("u_pickColor", id);

    Attribute sphereLocation = make_attribute("a_sphere", id);
    Attribute colorLocation = make_attribute("a_color", id);

    return SphereImpostorProgram{std::move(*program),
                                 modelMatrixLocation,
                                 viewMatrixLocation,
                                 projectionMatrixLocation,
                                 inverseModelViewMatrixLocation,
                                 normalMatrixLocation,
                                 invProjectionLocation,
                                 viewportSizeLocation,
                                 zeroToOneDepthLocation,
                                 lightPosLocation,
                                 lightColorLocation,
                                 fillLightDirectionLocation,
                                 fillLightColorLocation,
                                 ambientColorLocation,
                                 shininessLocation,
                                 lightAttenuationLocation,
                                 materialAmbientLocation,
                                 materialDiffuseLocation,
                                 materialSpecularLocation,
                                 pickModeLocation,
                                 pickColorLocation,
                                 sphereLocation,
                                 colorLocation};
}

} // namespace opengl
