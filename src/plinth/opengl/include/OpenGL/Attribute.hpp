#ifndef OPENGL_ATTRIBUTE_HPP
#define OPENGL_ATTRIBUTE_HPP

#include "OpenGL.hpp"
#include "OpenGL/Location.hpp"
#include "OpenGL/Programs/ProgramId.hpp"
#include "OpenGL/opengl_export.h"
#include <plinth/DLLWarnings.hpp>
#include <string_view>

namespace opengl {

RENDERER_SUPPRESS_STL_DLL_WARNINGS_BEGIN

class OPENGL_EXPORT Attribute {
    std::string_view m_name;
    Location m_location{};

  public:
    constexpr Attribute() noexcept = default;
    constexpr explicit Attribute(std::string_view name, Location location) noexcept;

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

OPENGL_EXPORT Attribute make_attribute(std::string_view name, ProgramId program);

} // namespace opengl

#endif // OPENGL_ATTRIBUTE_HPP
