#ifndef PLINTH_SPHERESIZESPACE_HPP
#define PLINTH_SPHERESIZESPACE_HPP

namespace renderer {

// Selects the space in which a sphere point's radius is measured.
enum class SphereSizeSpace {
    World,  // radius is in world units (spheres scale with zoom/distance)
    Screen, // radius is in pixels (spheres look constant on screen, like glPointSize)
};

} // namespace renderer

#endif // PLINTH_SPHERESIZESPACE_HPP
