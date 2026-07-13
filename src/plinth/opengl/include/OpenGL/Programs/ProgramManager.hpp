#ifndef OPENGL_PROGRAMMANAGER_HPP
#define OPENGL_PROGRAMMANAGER_HPP

#include "OpenGL/OpenGL.hpp"
#include "OpenGL/Programs/LineProgram.hpp"
#include "OpenGL/Programs/MeshProgram.hpp"
#include "OpenGL/Programs/PointProgram.hpp"
#include "OpenGL/Programs/PostProcessingProgram.hpp"
#include "OpenGL/opengl_export.h"

namespace opengl {

class DrawablesManager;

/** @brief Compiles and holds all programs available */
class OPENGL_EXPORT ProgramManager {
    LineProgram m_lineProgram;
    PointProgram m_pointProgram;
    MeshProgram m_meshProgram;
    PostProcessingProgram m_postProcessingProgram;

  public:
    ProgramManager() = default;
    ProgramManager(const ProgramManager&) = delete;
    ProgramManager& operator=(const ProgramManager&) = delete;
    ProgramManager(ProgramManager&&) noexcept = default;
    ProgramManager& operator=(ProgramManager&&) noexcept = default;
    ~ProgramManager() = default;

    [[nodiscard]]
    bool is_compiled() const {
        return m_lineProgram.is_valid() && m_pointProgram.is_valid() && m_meshProgram.is_valid() &&
               m_postProcessingProgram.is_valid();
    }

    void compile();

    [[nodiscard]]
    LineProgram& get_line_program() {
        return m_lineProgram;
    }
    [[nodiscard]]
    PointProgram& get_point_program() {
        return m_pointProgram;
    }

    [[nodiscard]]
    PostProcessingProgram& get_post_processing_program() {
        return m_postProcessingProgram;
    }

  private:
    friend class DrawablesManager;

    [[nodiscard]]
    MeshProgram& get_mesh_program() {
        return m_meshProgram;
    }
};

} // namespace opengl

#endif // OPENGL_PROGRAMMANAGER_HPP
