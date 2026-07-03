#ifndef OPENGL_CREATEPROGRAM_HPP
#define OPENGL_CREATEPROGRAM_HPP

#include "OpenGL/OpenGL.hpp"
#include "OpenGL/Programs/ProgramId.hpp"
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

namespace opengl {

enum class ProgramCreationFailureStage {
    Success = 0,
    VertexShaderCreation = 1,
    VertexShaderCompilation = 2,
    FragmentShaderCreation = 3,
    FragmentShaderCompilation = 4,
    ProgramCreation = 5,
    ProgramLinking = 6
};

OPENGL_EXPORT const std::error_category& program_creation_error_category() noexcept;
OPENGL_EXPORT std::error_code make_error_code(ProgramCreationFailureStage stage);

// Use this struct instead of std::expected only because Ubuntu GitHub running clang sanitizers creates abi problems
// right now.
struct ProgramCreationResult {
    std::variant<ProgramHandle, std::error_code> result;

    ProgramCreationResult(ProgramHandle program) noexcept
        : result{std::move(program)} {}

    ProgramCreationResult(std::error_code error) noexcept
        : result{error} {}

    [[nodiscard]]
    bool has_value() const noexcept {
        return std::holds_alternative<ProgramHandle>(result);
    }
    [[nodiscard]]
    explicit operator bool() const noexcept {
        return has_value();
    }

    [[nodiscard]]
    ProgramHandle& value() & {
        return std::get<ProgramHandle>(result);
    }
    [[nodiscard]]
    const ProgramHandle& value() const& {
        return std::get<ProgramHandle>(result);
    }
    [[nodiscard]]
    ProgramHandle&& value() && {
        return std::move(std::get<ProgramHandle>(result));
    }

    [[nodiscard]]
    std::error_code& error() & {
        return std::get<std::error_code>(result);
    }
    [[nodiscard]]
    const std::error_code& error() const& {
        return std::get<std::error_code>(result);
    }

    [[nodiscard]]
    ProgramHandle* operator->() {
        return &value();
    }
    [[nodiscard]]
    const ProgramHandle* operator->() const {
        return &value();
    }
    [[nodiscard]]
    ProgramHandle& operator*() & {
        return value();
    }
    [[nodiscard]]
    const ProgramHandle& operator*() const& {
        return value();
    }
    [[nodiscard]]
    ProgramHandle&& operator*() && {
        return std::move(value());
    }
};

OPENGL_EXPORT ProgramCreationResult create_program(const char* vertexShaderSource, const char* fragmentShaderSource);

} // namespace opengl

namespace std {
template <>
struct is_error_code_enum<opengl::ProgramCreationFailureStage> : true_type {};
} // namespace std

#endif // OPENGL_CREATEPROGRAM_HPP
