#ifndef PLINTH_SPHERESIZESPACE_HPP
#define PLINTH_SPHERESIZESPACE_HPP

namespace renderer {

// Selects the space in which a sphere point's size is measured.
enum class SphereSizeSpace {
    World,  // size is a radius in world units (spheres scale with zoom/distance)
    Screen, // size is a diameter in pixels (constant on screen, like glPointSize; matches line width)
};

} // namespace renderer

#endif // PLINTH_SPHERESIZESPACE_HPP
