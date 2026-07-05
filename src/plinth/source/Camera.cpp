#include <cmath>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <plinth/Assert.hpp>
#include <plinth/Camera.hpp>
#include <plinth/PickRay.hpp>

namespace renderer {

[[maybe_unused]]
static bool has_nan_value(const glm::dvec3& vec) {
    return std::isnan(vec[0]) || std::isnan(vec[1]) || std::isnan(vec[2]);
}

[[maybe_unused]]
static bool has_inf_value(const glm::dvec3& vec) {
    return std::isinf(vec[0]) || std::isinf(vec[1]) || std::isinf(vec[2]);
}

[[maybe_unused]]
static bool has_huge_value(const glm::dvec3& vec) {
    constexpr double hugeValue = 1e10;
    return std::abs(vec[0]) > hugeValue || std::abs(vec[1]) > hugeValue || std::abs(vec[2]) > hugeValue;
}

PickRay Camera::create_perspective_ray(double screenX, double screenY) const {
    // Remap the pixel coordinates (x, y) from the viewport space to the normalized device
    // coordinates (NDC) space, which ranges from [-1, +1]
    const auto [uNDC, vNDC] = screen_to_ndc(screenX, screenY);

    // Compute the tangent of the half field-of-view angle
    const double tangent = std::tan(glm::radians(m_perspective.fov) / 2.0);

    glm::dvec3 cameraGaze = gaze();
    const glm::dvec3 right = glm::normalize(glm::cross(cameraGaze, get_vertical()));
    const glm::dvec3 upward = glm::normalize(glm::cross(right, cameraGaze));

    // Adjust the ray direction for vertical field of view
    cameraGaze += right * tangent * uNDC * m_viewport.get_aspect_ratio();
    cameraGaze += upward * tangent * vNDC;

    cameraGaze = glm::normalize(cameraGaze);

    RENDERER_ASSERT(!has_nan_value(cameraGaze));
    RENDERER_ASSERT(!has_nan_value(get_position()));
    RENDERER_ASSERT(!has_inf_value(cameraGaze));
    RENDERER_ASSERT(!has_inf_value(get_position()));
    RENDERER_ASSERT(!has_huge_value(cameraGaze));
    RENDERER_ASSERT(!has_huge_value(get_position()));

    return {to_linal(get_position()), to_linal(cameraGaze)};
}

PickRay Camera::create_orthographic_ray(double screenX, double screenY) const {
    // Remap the pixel coordinates (x, y) from the viewport space to the normalized device
    // coordinates (NDC) space, which ranges from [-1, +1]
    const auto [uNDC, vNDC] = screen_to_ndc(screenX, screenY);

    const glm::dvec3 cameraGaze = gaze();
    const glm::dvec3 right = glm::normalize(glm::cross(cameraGaze, get_vertical()));
    const glm::dvec3 upward = glm::normalize(glm::cross(right, cameraGaze));

    const double halfWidth = m_orthographic.width / 2.0 * m_viewport.get_aspect_ratio();
    const double halfHeight = m_orthographic.height / 2.0;

    // Adjust the ray origin for the orthogonal projection
    glm::dvec3 rayOrigin = get_position();
    rayOrigin += right * uNDC * halfWidth;
    rayOrigin += upward * vNDC * halfHeight;

    // The direction of the ray in an orthographic projection is the same for all rays,
    // and it's parallel to the cameraGaze vector. Here, the cameraGaze vector is already normalized.
    return {to_linal(rayOrigin), to_linal(cameraGaze)};
}

} // namespace renderer
