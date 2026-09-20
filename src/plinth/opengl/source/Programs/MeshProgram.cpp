#include "OpenGL/Programs/MeshProgram.hpp"

#include "OpenGL/ErrorReporting.hpp"
#include "OpenGL/FrameUniforms.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"
#include "plinth/Assert.hpp"

#include <format>
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
        report_error(std::format("Error category: '{}';Error code: '{}'; Error message: '{}'",
                                 programCreation.error().category().name(),
                                 programCreation.error().value(),
                                 programCreation.error().message()));
        RENDERER_ASSERT(false);
        return {};
    }
    ProgramHandle program = std::move(programCreation).value();
    ProgramId id = program.get_id();
    RENDERER_ASSERT(id.get_value() != 0);

    MeshProgramInput input = {.m_modelMatrix = make_uniform("u_model", id),
                              .m_normalMatrix = make_uniform("u_normalMatrix", id),
                              .m_posLocation = make_attribute("a_vertex", id),
                              .m_colorLocation = make_attribute("a_color", id),
                              .m_normalLocation = make_attribute("a_normal", id),
                              .m_texCoordLocation = make_attribute("a_texCoord", id),
                              .hasAlbedoTexture = make_uniform("u_hasAlbedoTexture", id),
                              .albedoTexture = make_uniform("u_albedoTexture", id),
                              .m_pickMode = make_uniform("u_pickMode", id),
                              .m_pickColor = make_uniform("u_pickColor", id)};

    bind_uniform_block(id, "FrameBlock", kFrameUniformBinding);

    return MeshProgram{std::move(program), input};
}

} // namespace opengl
