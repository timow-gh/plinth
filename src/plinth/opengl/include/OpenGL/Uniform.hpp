#ifndef OPENGL_UNIFORM_HPP
#define OPENGL_UNIFORM_HPP

#include "OpenGL.hpp"
#include "OpenGL/Location.hpp"
#include "OpenGL/Programs/ProgramId.hpp"
#include "OpenGL/opengl_export.h"
#include <plinth/DLLWarnings.hpp>
#include <string_view>

namespace opengl {

RENDERER_SUPPRESS_STL_DLL_WARNINGS_BEGIN

class OPENGL_EXPORT Uniform {
    std::string_view m_name;
    Location m_location{};

  public:
    constexpr Uniform() noexcept = default;
    explicit Uniform(std::string_view name, Location location) noexcept;

    [[nodiscard]]
    constexpr Location get_location() const noexcept {
        return m_location;
    }
    [[nodiscard]]
    constexpr std::string_view get_name() const noexcept {
        return m_name;
    }
};

RENDERER_SUPPRESS_STL_DLL_WARNINGS_END

Uniform make_uniform(std::string_view name, ProgramId program);

} // namespace opengl

#endif // OPENGL_UNIFORM_HPP
