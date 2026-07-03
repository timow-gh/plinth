#include "OpenGL/Programs/MeshProgram.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"
#include <Renderer/Assert.hpp>
#include <utility>

namespace opengl {

MeshProgram::MeshProgram(ProgramHandle program, const MeshProgramInput& input) noexcept
    : m_program{std::move(program)}
    , m_input{input} {
    RENDERER_ASSERT(m_program.is_valid());
    assert_mesh_program_input(input);
}

ProgramId MeshProgram::get_id() const {
    return m_program.get_id();
}

void MeshProgram::use() const {
    glUseProgram(m_program.get_value());
}

MeshProgram make_mesh_program() {
    ProgramCreationResult programCreation =
        create_program(mesh_vertex_shader_source().data(), mesh_fragment_shader_source().data());
    if (!programCreation.has_value()) {
        RENDERER_ASSERT(false);
        return {};
    }
    ProgramHandle program = std::move(programCreation).value();
    ProgramId id = program.get_id();
    RENDERER_ASSERT(id.get_value() != 0);

    MeshProgramInput input = {.m_modelMatrix = make_uniform("u_model", id),
                              .m_viewMatrix = make_uniform("u_view", id),
                              .m_projectionMatrix = make_uniform("u_projection", id),
                              .m_normalMatrix = make_uniform("u_normalMatrix", id),
                              .m_posLocation = make_attribute("a_vertex", id),
                              .m_colorLocation = make_attribute("a_color", id),
                              .m_normalLocation = make_attribute("a_normal", id),
                              .m_lightPos = make_uniform("u_lightPos", id),
                              .viewPos = make_uniform("u_viewPos", id),
                              .lightColor = make_uniform("u_lightColor", id),
                              .fillLightDirection = make_uniform("u_fillLightDirection", id),
                              .fillLightColor = make_uniform("u_fillLightColor", id),
                              .ambientColor = make_uniform("u_ambientColor", id),
                              .shininess = make_uniform("u_shininess", id)};

    return MeshProgram{std::move(program), input};
}

} // namespace opengl
