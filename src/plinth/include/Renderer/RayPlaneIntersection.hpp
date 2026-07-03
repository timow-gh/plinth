#ifndef RENDERER_RAYPLANEINTERSECTION_HPP
#define RENDERER_RAYPLANEINTERSECTION_HPP

#include <Renderer/Assert.hpp>
#include <Renderer/Plane.hpp>
#include <linal/linal.hpp>

namespace renderer {

/** @brief Computes the intersection of a ray with a plane.
 *  @param rayOrigin The origin of the ray.
 *  @param rayDir The direction of the ray.
 *  @param planeCoefficients The coefficients of the plane equation.
 *  @param result The intersection point.
 *  @return True if the ray intersects the plane, false otherwise.
 */
[[nodiscard]]
inline bool ray_plane_intersection(const linal::double3& rayOrigin,
                                   const linal::double3& rayDir,
                                   const Plane& plane,
                                   linal::double3& result,
                                   double eps = 1e-6) {
    RENDERER_ASSERT(!linal::is_zero(rayDir));
    const linal::double3 planeNormal = plane.get_normal();
    const double dirDot = linal::dot(rayDir, planeNormal);

    // Check if the ray is parallel to the plane
    if (std::abs(dirDot) < eps) {
        return false;
    }

    const linal::double3 originDistDir = plane.get_origin() - rayOrigin;
    // Calculate the ray parameter (t) using the dot product of originDistDir and planeNormal divided
    // by the dot product of rayDir and planeNormal
    const double rayParam = linal::dot(originDistDir, planeNormal) / dirDot;

    // If the rayParam is negative, the intersection is behind the ray origin, so no intersection
    if (rayParam < 0.0)
        return false;

    // Calculate the intersection point
    result = rayOrigin + rayDir * rayParam;
    return true;
}

} // namespace renderer

#endif // RENDERER_RAYPLANEINTERSECTION_HPP
