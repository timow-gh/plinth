#ifndef OPENGL_SOURCE_PROGRAMS_PROGRAMOPENGL_HPP
#define OPENGL_SOURCE_PROGRAMS_PROGRAMOPENGL_HPP

#include "OpenGL/OpenGL.hpp"
#include <string>

namespace opengl::program_opengl {

[[nodiscard]]
GLuint create_shader(GLenum shaderType);
void set_shader_source(GLuint shader, const char* shaderSource);
void compile_shader(GLuint shader);
[[nodiscard]]
bool get_shader_compile_status(GLuint shader);
[[nodiscard]]
std::string get_shader_info_log(GLuint shader);
void delete_shader(GLuint shader);

[[nodiscard]]
GLuint create_program();
void attach_shader(GLuint program, GLuint shader);
void link_program(GLuint program);
[[nodiscard]]
bool get_program_link_status(GLuint program);
[[nodiscard]]
std::string get_program_info_log(GLuint program);
void delete_program(GLuint program);

} // namespace opengl::program_opengl

#endif // OPENGL_SOURCE_PROGRAMS_PROGRAMOPENGL_HPP
