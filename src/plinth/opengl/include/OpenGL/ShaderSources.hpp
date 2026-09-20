#ifndef OPENGL_SHADERSOURCES_HPP
#define OPENGL_SHADERSOURCES_HPP

#include <filesystem>
#include <string>

namespace opengl {

// Declaration of the shared std140 `FrameBlock` uniform block (no instance name, so members are
// addressed directly). Prepend it after the `#version` line of any shader stage that reads
// frame-constant camera/lighting data. See FrameUniforms.hpp for the matching C++ layout.
std::string frame_uniform_block_glsl();

std::string line_vertex_shader_source();
std::string line_fragment_shader_source();
std::string point_color_vertex_shader_source();
std::string point_color_fragment_shader_source();

std::string mesh_vertex_shader_source();
std::string mesh_fragment_shader_source();

std::string sphere_impostor_vertex_shader_source();
std::string sphere_impostor_fragment_shader_source();

std::string post_processing_vertex_shader_source();
std::string post_processing_fragment_shader_source();
std::string fxaa_vertex_shader_source();
std::string fxaa_fragment_shader_source();

} // namespace opengl

#endif // OPENGL_SHADERSOURCES_HPP
