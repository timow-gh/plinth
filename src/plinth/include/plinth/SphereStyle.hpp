#ifndef PLINTH_SPHERESTYLE_HPP
#define PLINTH_SPHERESTYLE_HPP

#include "plinth/SphereSizeSpace.hpp"

namespace renderer {

/// Styling parameters for sphere-point drawables.
///
/// Groups per-drawable style properties that apply uniformly to a sphere-point drawable.
/// Kept separate from the per-instance center/radius/color data so it can grow (e.g. an
/// outline or a default radius) without changing the point data contract.
struct SphereStyle {
    /// Selects how each per-instance size value is interpreted.
    ///   World  — value is a radius in world units (spheres scale with zoom/distance).
    ///   Screen — value is a diameter in pixels (matches StrokeStyle::lineWidth); each sphere
    ///            holds a roughly constant on-screen size regardless of distance or zoom, like
    ///            glPointSize.
    SphereSizeSpace sizeSpace{SphereSizeSpace::Screen};
};

} // namespace renderer

#endif // PLINTH_SPHERESTYLE_HPP
