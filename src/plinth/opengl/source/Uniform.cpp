#include "OpenGL/Uniform.hpp"
#include <cstdlib>
#include <plinth/Assert.hpp>
#include <print>

namespace opengl {

Uniform::Uniform(std::string_view name, Location location) noexcept
    : m_name{name}
    , m_location{location} {
    RENDERER_ASSERT(!name.empty());
    RENDERER_ASSERT(location.get_value() != -1);
}

Uniform make_uniform(std::string_view name, ProgramId program) {
    RENDERER_ASSERT(!name.empty());
    RENDERER_ASSERT(program.get_value() != 0);

    Location location{glGetUniformLocation(program.get_value(), name.data())};
    if (location.get_value() == -1) {
        RENDERER_ASSERT(false);
        std::print("Uniform {} location not found\n", name);
        std::abort();
    }

    return Uniform{name, location};
}

} // namespace opengl
