#ifndef OPENGL_POSTPROCESSINGPROGRAM_HPP
#define OPENGL_POSTPROCESSINGPROGRAM_HPP

#include "OpenGL/Programs/ProgramId.hpp"
#include "OpenGL/Uniform.hpp"
#include <plinth/Assert.hpp>

namespace opengl {

struct OPENGL_EXPORT PostProcessingProgramInput {
    Uniform m_sceneTexture;
};

constexpr void assert_post_processing_program_input([[maybe_unused]] const PostProcessingProgramInput& input) noexcept {
    RENDERER_ASSERT(input.m_sceneTexture.get_location().get_value() != -1);
}

class OPENGL_EXPORT PostProcessingProgram {
    ProgramHandle m_program;

    PostProcessingProgramInput m_input;

  public:
    PostProcessingProgram() noexcept = default;
    PostProcessingProgram(ProgramHandle program, const PostProcessingProgramInput& input) noexcept;

    PostProcessingProgram(const PostProcessingProgram&) = delete;
    PostProcessingProgram& operator=(const PostProcessingProgram&) = delete;
    PostProcessingProgram(PostProcessingProgram&& other) noexcept {
        m_program = std::move(other.m_program);
        m_input = std::move(other.m_input);
    }
    PostProcessingProgram& operator=(PostProcessingProgram&& other) noexcept {
        if (this != &other) {
            m_program = std::move(other.m_program);
            m_input = std::move(other.m_input);
        }
        return *this;
    }
    ~PostProcessingProgram() = default;

    [[nodiscard]]
    bool is_valid() const noexcept {
        return m_program.is_valid();
    }

    [[nodiscard]]
    ProgramId get_id() const;

    [[nodiscard]]
    constexpr Location get_scene_texture_location() const noexcept {
        return m_input.m_sceneTexture.get_location();
    }

    void use() const;
};

OPENGL_EXPORT PostProcessingProgram make_post_processing_program();

} // namespace opengl

#endif // OPENGL_POSTPROCESSINGPROGRAM_HPP
