#include "OpenGL/Attribute.hpp"
#include "OpenGL/ErrorReporting.hpp"
#include <plinth/Assert.hpp>
#include <cstdlib>
#include <format>

namespace opengl {

constexpr Attribute::Attribute(std::string_view name, Location location) noexcept
    : m_name{name}
    , m_location{location} {
    RENDERER_ASSERT(!name.empty());
    RENDERER_ASSERT(location.get_value() != -1);
}

Attribute make_attribute(std::string_view name, ProgramId program) {
    RENDERER_ASSERT(program.get_value() != 0);
    RENDERER_ASSERT(!name.empty());

    Location location{glGetAttribLocation(program.get_value(), name.data())};
    if (location.get_value() == -1) {
        RENDERER_ASSERT(false);
        report_error(std::format("Attribute {} not found", name));
        abort();
    }

    return Attribute{name, location};
}

} // namespace opengl
