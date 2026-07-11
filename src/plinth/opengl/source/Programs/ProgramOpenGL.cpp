#include "ProgramOpenGL.hpp"
#include "OpenGL/ErrorReporting.hpp"

namespace opengl::program_opengl {

namespace {

std::string get_shader_log(GLuint shader, GLenum lengthParameter) {
    GLint logLength{0};
    glGetShaderiv(shader, lengthParameter, &logLength);
    if (logLength <= 0) {
        return {};
    }

    std::string log(static_cast<std::size_t>(logLength), '\0');
    GLsizei writtenLength{0};
    glGetShaderInfoLog(shader, logLength, &writtenLength, log.data());
    log.resize(static_cast<std::size_t>(writtenLength));
    return log;
}

std::string get_program_log(GLuint program, GLenum lengthParameter) {
    GLint logLength{0};
    glGetProgramiv(program, lengthParameter, &logLength);
    if (logLength <= 0) {
        return {};
    }

    std::string log(static_cast<std::size_t>(logLength), '\0');
    GLsizei writtenLength{0};
    glGetProgramInfoLog(program, logLength, &writtenLength, log.data());
    log.resize(static_cast<std::size_t>(writtenLength));
    return log;
}

} // namespace

GLuint create_shader(GLenum shaderType) {
    return glCreateShader(shaderType);
}

void set_shader_source(GLuint shader, const char* shaderSource) {
    glShaderSource(shader, 1, &shaderSource, nullptr);
}

void compile_shader(GLuint shader) {
    glCompileShader(shader);
}

bool get_shader_compile_status(GLuint shader) {
    GLint success{GL_FALSE};
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    return success == GL_TRUE;
}

std::string get_shader_info_log(GLuint shader) {
    return get_shader_log(shader, GL_INFO_LOG_LENGTH);
}

void delete_shader(GLuint shader) {
    glDeleteShader(shader);
}

GLuint create_program() {
    return glCreateProgram();
}

void attach_shader(GLuint program, GLuint shader) {
    glAttachShader(program, shader);
}

void link_program(GLuint program) {
    glLinkProgram(program);
    check_gl_errors("after glLinkProgram");
}

bool get_program_link_status(GLuint program) {
    GLint success{GL_FALSE};
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    return success == GL_TRUE;
}

std::string get_program_info_log(GLuint program) {
    return get_program_log(program, GL_INFO_LOG_LENGTH);
}

void delete_program(GLuint program) {
    glDeleteProgram(program);
}

} // namespace opengl::program_opengl
