#include "OpenGL/Programs/ProgramManager.hpp"

namespace opengl {

void ProgramManager::compile() {
    m_lineProgram = make_line_program();
    m_pointProgram = make_point_program();
    m_meshProgram = make_mesh_program();
    m_sphereImpostorProgram = make_sphere_impostor_program();
}

} // namespace opengl
