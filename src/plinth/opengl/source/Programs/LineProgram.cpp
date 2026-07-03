#include "OpenGL/Programs/LineProgram.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"
#include <Renderer/Assert.hpp>
#include <print>
#include <string>
#include <utility>

namespace opengl {

LineProgram::LineProgram(ProgramHandle program,
                         Uniform mvpLocation,
                         Attribute vertexLocation,
                         Attribute colorLocation) noexcept
    : m_program{std::move(program)}
    , m_mvpLocation{mvpLocation}
    , m_vertexLocation{vertexLocation}
    , m_colorLocation{colorLocation} {
    RENDERER_ASSERT(m_program.is_valid());
    RENDERER_ASSERT(mvpLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(vertexLocation.get_location().get_value() != -1);
    RENDERER_ASSERT(colorLocation.get_location().get_value() != -1);
}

void LineProgram::use() const {
    glUseProgram(m_program.get_value());
}

opengl::LineProgram make_line_program() {
    const std::string vertexShaderSource = line_vertex_shader_source();
    const std::string fragmentShaderSource = line_fragment_shader_source();
    ProgramCreationResult program = create_program(vertexShaderSource.c_str(), fragmentShaderSource.c_str());
    if (!program) {
        std::print(stderr,
                   "Error category: '{}';Error code: '{}'; Error message: '{}'\n",
                   program.error().category().name(),
                   program.error().value(),
                   program.error().message());
        RENDERER_ASSERT(false);
        return {};
    }

    ProgramId id = program->get_id();
    RENDERER_ASSERT(id.get_value() != 0);
    Uniform mvpLocation = make_uniform("u_MVP", id);
    Attribute vertexLocation = make_attribute("a_vertex", id);
    Attribute colorLocation = make_attribute("a_color", id);
    return LineProgram{std::move(*program), mvpLocation, vertexLocation, colorLocation};
}

} // namespace opengl
