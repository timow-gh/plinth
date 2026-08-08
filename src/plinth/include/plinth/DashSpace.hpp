#ifndef PLINTH_DASHSPACE_HPP
#define PLINTH_DASHSPACE_HPP

namespace renderer {

// Selects the space in which dash and gap lengths are measured.
enum class DashSpace {
    World,  // dash/gap lengths are constant in world units (dashes scale with zoom)
    Screen, // dash/gap lengths are constant in pixels (dashes look constant on screen)
};

} // namespace renderer

#endif // PLINTH_DASHSPACE_HPP
