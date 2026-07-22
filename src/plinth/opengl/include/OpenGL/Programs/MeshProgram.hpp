#ifndef OPENGL_MESHPROGRAM_HPP
#define OPENGL_MESHPROGRAM_HPP

#include "OpenGL/Attribute.hpp"
#include "OpenGL/Programs/ProgramId.hpp"
#include "OpenGL/Uniform.hpp"
#include <plinth/Assert.hpp>

namespace opengl {

struct OPENGL_EXPORT MeshProgramInput {
    // Vertex shader uniforms and attributes
    Uniform m_modelMatrix;
    Uniform m_viewMatrix;
    Uniform m_projectionMatrix;
    Uniform m_normalMatrix;     // Normal matrix for phong lighting, i.e. the inverse transpose of the
                                // model matrix
    Attribute m_posLocation;    // Position of the vertices attribute
    Attribute m_colorLocation;  // Position of the color attribute
    Attribute m_normalLocation; // Position of the normal attribute for phong lighting
    Attribute m_texCoordLocation;

    // Fragment shader uniforms and attributes
    Uniform m_lightPos; // Position of the light source
    Uniform viewPos;    // Position of the camera
    Uniform lightColor;
    Uniform fillLightDirection;
    Uniform fillLightColor;
    Uniform ambientColor;
    Uniform shininess;
    Uniform hasAlbedoTexture;
    Uniform albedoTexture;
    Uniform lightAttenuation;
    Uniform materialAmbient;
    Uniform materialDiffuse;
    Uniform materialSpecular;
};

constexpr void assert_mesh_program_input([[maybe_unused]] const MeshProgramInput& input) noexcept {
    RENDERER_ASSERT(input.m_modelMatrix.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_viewMatrix.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_projectionMatrix.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_normalMatrix.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_posLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_colorLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_normalLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_texCoordLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_lightPos.get_location().get_value() != -1);
    RENDERER_ASSERT(input.viewPos.get_location().get_value() != -1);
    RENDERER_ASSERT(input.lightColor.get_location().get_value() != -1);
    RENDERER_ASSERT(input.fillLightDirection.get_location().get_value() != -1);
    RENDERER_ASSERT(input.fillLightColor.get_location().get_value() != -1);
    RENDERER_ASSERT(input.ambientColor.get_location().get_value() != -1);
    RENDERER_ASSERT(input.shininess.get_location().get_value() != -1);
    RENDERER_ASSERT(input.hasAlbedoTexture.get_location().get_value() != -1);
    RENDERER_ASSERT(input.albedoTexture.get_location().get_value() != -1);
    RENDERER_ASSERT(input.lightAttenuation.get_location().get_value() != -1);
    RENDERER_ASSERT(input.materialAmbient.get_location().get_value() != -1);
    RENDERER_ASSERT(input.materialDiffuse.get_location().get_value() != -1);
    RENDERER_ASSERT(input.materialSpecular.get_location().get_value() != -1);
}

class OPENGL_EXPORT MeshProgram {
    ProgramHandle m_program;

    MeshProgramInput m_input;

  public:
    MeshProgram() noexcept = default;
    MeshProgram(ProgramHandle program, const MeshProgramInput& input) noexcept;

    MeshProgram(const MeshProgram&) = delete;
    MeshProgram& operator=(const MeshProgram&) = delete;
    MeshProgram(MeshProgram&& other) noexcept {
        m_program = std::move(other.m_program);
        m_input = std::move(other.m_input);
    }
    MeshProgram& operator=(MeshProgram&& other) noexcept {
        if (this != &other) {
            m_program = std::move(other.m_program);
            m_input = std::move(other.m_input);
        }
        return *this;
    }
    ~MeshProgram() = default;

    [[nodiscard]]
    bool is_valid() const noexcept {
        return m_program.is_valid();
    }

    [[nodiscard]]
    ProgramId get_id() const;

    [[nodiscard]]
    constexpr Location get_model_matrix_location() const noexcept {
        return m_input.m_modelMatrix.get_location();
    }
    [[nodiscard]]
    constexpr Location get_view_matrix_location() const noexcept {
        return m_input.m_viewMatrix.get_location();
    }
    [[nodiscard]]
    constexpr Location get_projection_matrix_location() const noexcept {
        return m_input.m_projectionMatrix.get_location();
    }
    [[nodiscard]]
    constexpr Location get_normal_matrix_location() const noexcept {
        return m_input.m_normalMatrix.get_location();
    }
    [[nodiscard]]
    constexpr Location get_pos_location() const noexcept {
        return m_input.m_posLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_color_location() const noexcept {
        return m_input.m_colorLocation.get_location();
    }
    [[nodiscard]]
    constexpr Location get_normal_location() const noexcept {
        return m_input.m_normalLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_tex_coord_location() const noexcept { return m_input.m_texCoordLocation.get_location(); }

    [[nodiscard]]
    constexpr Location get_light_pos_location() const noexcept {
        return m_input.m_lightPos.get_location();
    }
    [[nodiscard]]
    constexpr Location get_view_pos_location() const noexcept {
        return m_input.viewPos.get_location();
    }
    [[nodiscard]]
    constexpr Location get_light_color_location() const noexcept {
        return m_input.lightColor.get_location();
    }
    [[nodiscard]]
    constexpr Location get_fill_light_direction_location() const noexcept {
        return m_input.fillLightDirection.get_location();
    }
    [[nodiscard]]
    constexpr Location get_fill_light_color_location() const noexcept {
        return m_input.fillLightColor.get_location();
    }
    [[nodiscard]]
    constexpr Location get_ambient_color_location() const noexcept {
        return m_input.ambientColor.get_location();
    }
    [[nodiscard]]
    constexpr Location get_shininess_location() const noexcept {
        return m_input.shininess.get_location();
    }
    [[nodiscard]] constexpr Location get_has_albedo_texture_location() const noexcept { return m_input.hasAlbedoTexture.get_location(); }
    [[nodiscard]] constexpr Location get_albedo_texture_location() const noexcept { return m_input.albedoTexture.get_location(); }
    [[nodiscard]] constexpr Location get_light_attenuation_location() const noexcept { return m_input.lightAttenuation.get_location(); }
    [[nodiscard]] constexpr Location get_material_ambient_location() const noexcept { return m_input.materialAmbient.get_location(); }
    [[nodiscard]] constexpr Location get_material_diffuse_location() const noexcept { return m_input.materialDiffuse.get_location(); }
    [[nodiscard]] constexpr Location get_material_specular_location() const noexcept { return m_input.materialSpecular.get_location(); }

    void use() const;
};

OPENGL_EXPORT MeshProgram make_mesh_program();

} // namespace opengl

#endif // OPENGL_MESHPROGRAM_HPP
