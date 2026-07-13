#include "OpenGL/Programs/PostProcessingProgram.hpp"
#include "OpenGL/ErrorReporting.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"
#include <format>
#include <plinth/Assert.hpp>
#include <utility>

namespace opengl {

PostProcessingProgram::PostProcessingProgram(ProgramHandle program, const PostProcessingProgramInput& input) noexcept
    : m_program{std::move(program)}
    , m_input{input} {
    RENDERER_ASSERT(m_program.is_valid());
    assert_post_processing_program_input(input);
}

ProgramId PostProcessingProgram::get_id() const {
    return m_program.get_id();
}

void PostProcessingProgram::use() const {
    glUseProgram(m_program.get_value());
}

PostProcessingProgram make_post_processing_program() {
    ProgramCreationResult programCreation =
        create_program(post_processing_vertex_shader_source().data(),
                       post_processing_fragment_shader_source().data());
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

    PostProcessingProgramInput input = {.m_sceneTexture = make_uniform("u_sceneTexture", id)};

    return PostProcessingProgram{std::move(program), input};
}

} // namespace opengl
