#ifndef OPENGL_SHADERSOURCES_HPP
#define OPENGL_SHADERSOURCES_HPP

#include <filesystem>
#include <string>

namespace opengl {

std::string line_vertex_shader_source();
std::string line_fragment_shader_source();
std::string point_color_vertex_shader_source();
std::string point_color_fragment_shader_source();

std::string mesh_vertex_shader_source();
std::string mesh_fragment_shader_source();

std::string post_processing_vertex_shader_source();
std::string post_processing_fragment_shader_source();

} // namespace opengl

#endif // OPENGL_SHADERSOURCES_HPP
