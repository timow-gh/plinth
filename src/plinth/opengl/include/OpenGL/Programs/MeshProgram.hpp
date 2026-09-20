#ifndef OPENGL_MESHPROGRAM_HPP
#define OPENGL_MESHPROGRAM_HPP

#include "OpenGL/Attribute.hpp"
#include "OpenGL/Programs/ProgramId.hpp"
#include "OpenGL/Uniform.hpp"
#include "plinth/Assert.hpp"

namespace opengl {

struct OPENGL_EXPORT MeshProgramInput {
    // Vertex shader uniforms and attributes
    Uniform m_modelMatrix;
    Uniform m_normalMatrix;     // Normal matrix for phong lighting, i.e. the inverse transpose of the
                                // model matrix
    Attribute m_posLocation;    // Position of the vertices attribute
    Attribute m_colorLocation;  // Position of the color attribute
    Attribute m_normalLocation; // Position of the normal attribute for phong lighting
    Attribute m_texCoordLocation;

    // Fragment shader uniforms (frame-constant view/projection/lighting live in the FrameBlock UBO)
    Uniform hasAlbedoTexture;
    Uniform albedoTexture;

    // Color-ID picking: when m_pickMode is set the fragment shader outputs m_pickColor instead of
    // the lit surface color.
    Uniform m_pickMode;
    Uniform m_pickColor;
};

inline void assert_mesh_program_input([[maybe_unused]] const MeshProgramInput& input) noexcept {
    RENDERER_ASSERT(input.m_modelMatrix.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_normalMatrix.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_posLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_colorLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_normalLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_texCoordLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(input.hasAlbedoTexture.get_location().get_value() != -1);
    RENDERER_ASSERT(input.albedoTexture.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_pickMode.get_location().get_value() != -1);
    RENDERER_ASSERT(input.m_pickColor.get_location().get_value() != -1);
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

    [[nodiscard]] bool is_valid() const noexcept { return m_program.is_valid(); }

    [[nodiscard]] ProgramId get_id() const;

    [[nodiscard]] constexpr Location get_model_matrix_location() const noexcept {
        return m_input.m_modelMatrix.get_location();
    }
    [[nodiscard]] constexpr Location get_normal_matrix_location() const noexcept {
        return m_input.m_normalMatrix.get_location();
    }
    [[nodiscard]] constexpr Location get_pos_location() const noexcept { return m_input.m_posLocation.get_location(); }
    [[nodiscard]] constexpr Location get_color_location() const noexcept {
        return m_input.m_colorLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_normal_location() const noexcept {
        return m_input.m_normalLocation.get_location();
    }
    [[nodiscard]] constexpr Location get_tex_coord_location() const noexcept {
        return m_input.m_texCoordLocation.get_location();
    }

    [[nodiscard]] constexpr Location get_has_albedo_texture_location() const noexcept {
        return m_input.hasAlbedoTexture.get_location();
    }
    [[nodiscard]] constexpr Location get_albedo_texture_location() const noexcept {
        return m_input.albedoTexture.get_location();
    }
    [[nodiscard]] constexpr Location get_pick_mode_location() const noexcept {
        return m_input.m_pickMode.get_location();
    }
    [[nodiscard]] constexpr Location get_pick_color_location() const noexcept {
        return m_input.m_pickColor.get_location();
    }

    void use() const;
};

OPENGL_EXPORT MeshProgram make_mesh_program();

} // namespace opengl

#endif // OPENGL_MESHPROGRAM_HPP
