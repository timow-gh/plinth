#include "OpenGL/Attribute.hpp"
#include <plinth/Assert.hpp>
#include <cstdlib>
#include <print>

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
        std::print(stderr, "Attribute {} not found\n", name);
        abort();
    }

    return Attribute{name, location};
}

} // namespace opengl
