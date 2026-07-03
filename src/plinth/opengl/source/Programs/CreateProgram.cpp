#include "OpenGL/Programs/CreateProgram.hpp"
#include "ProgramOpenGL.hpp"
#include <print>
#include <utility>

namespace opengl {

namespace {

class ProgramCreationErrorCategory : public std::error_category {
  public:
    [[nodiscard]]
    const char* name() const noexcept override {
        return "opengl.program_creation";
    }

    [[nodiscard]]
    std::string message(int ev) const override {
        switch (static_cast<ProgramCreationFailureStage>(ev)) {
        case ProgramCreationFailureStage::Success:                   return "success";
        case ProgramCreationFailureStage::VertexShaderCreation:      return "vertex shader creation failed";
        case ProgramCreationFailureStage::VertexShaderCompilation:   return "vertex shader compilation failed";
        case ProgramCreationFailureStage::FragmentShaderCreation:    return "fragment shader creation failed";
        case ProgramCreationFailureStage::FragmentShaderCompilation: return "fragment shader compilation failed";
        case ProgramCreationFailureStage::ProgramCreation:           return "program creation failed";
        case ProgramCreationFailureStage::ProgramLinking:            return "program linking failed";
        default:                                                     return "unknown program creation error";
        }
    }
};

} // namespace

const std::error_category& program_creation_error_category() noexcept {
    static const ProgramCreationErrorCategory instance;
    return instance;
}

std::error_code make_error_code(ProgramCreationFailureStage stage) {
    return {static_cast<int>(stage), program_creation_error_category()};
}

ProgramCreationResult create_program(const char* vertexShaderSource, const char* fragmentShaderSource) {
    const GLuint vertexShader = program_opengl::create_shader(GL_VERTEX_SHADER);
    if (vertexShader == 0) {
        return make_error_code(ProgramCreationFailureStage::VertexShaderCreation);
    }
    struct ShaderGuard {
        GLuint id;

        explicit ShaderGuard(GLuint shaderId)
            : id(shaderId) {}

        ShaderGuard(const ShaderGuard&) = delete;
        ShaderGuard& operator=(const ShaderGuard&) = delete;
        ShaderGuard(ShaderGuard&&) = delete;
        ShaderGuard& operator=(ShaderGuard&&) = delete;

        ~ShaderGuard() { program_opengl::delete_shader(id); }
    };
    const ShaderGuard vertexShaderDeleter{vertexShader};
    program_opengl::set_shader_source(vertexShader, vertexShaderSource);
    program_opengl::compile_shader(vertexShader);

    if (!program_opengl::get_shader_compile_status(vertexShader)) {
        std::print(stderr, "Vertex shader compilation failed: {}\n", program_opengl::get_shader_info_log(vertexShader));
        return make_error_code(ProgramCreationFailureStage::VertexShaderCompilation);
    }

    const GLuint fragmentShader = program_opengl::create_shader(GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) {
        return make_error_code(ProgramCreationFailureStage::FragmentShaderCreation);
    }
    const ShaderGuard fragmentShaderDeleter{fragmentShader};
    program_opengl::set_shader_source(fragmentShader, fragmentShaderSource);
    program_opengl::compile_shader(fragmentShader);

    if (!program_opengl::get_shader_compile_status(fragmentShader)) {
        std::print(stderr,
                   "Fragment shader compilation failed: {}\n",
                   program_opengl::get_shader_info_log(fragmentShader));
        return make_error_code(ProgramCreationFailureStage::FragmentShaderCompilation);
    }

    const GLuint programId = program_opengl::create_program();
    if (programId == 0) {
        return make_error_code(ProgramCreationFailureStage::ProgramCreation);
    }

    program_opengl::attach_shader(programId, vertexShader);
    program_opengl::attach_shader(programId, fragmentShader);
    program_opengl::link_program(programId);

    if (!program_opengl::get_program_link_status(programId)) {
        std::string infoLog = program_opengl::get_program_info_log(programId);
        std::print(stderr, "Program linking failed: {}\n", infoLog);
        program_opengl::delete_program(programId);
        return make_error_code(ProgramCreationFailureStage::ProgramLinking);
    }

    return ProgramHandle{ProgramId{programId}};
}
} // namespace opengl
