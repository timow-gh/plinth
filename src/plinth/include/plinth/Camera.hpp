#ifndef RENDERER_CAMERA_HPP
#define RENDERER_CAMERA_HPP

#include <plinth/Assert.hpp>
#include <plinth/CameraProjectionType.hpp>
#include <plinth/PickRay.hpp>
#include <plinth/Viewport.hpp>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace renderer {

inline auto to_glm(const linal::double3& vec) {
    return glm::dvec3{vec[0], vec[1], vec[2]};
}

inline auto to_glm(const linal::float3& vec) {
    return glm::vec3{vec[0], vec[1], vec[2]};
}

inline auto to_linal(const glm::dvec3& vec) {
    return linal::double3{vec.x, vec.y, vec.z};
}

inline auto to_linal(const glm::vec3& vec) {
    return linal::float3{vec.x, vec.y, vec.z};
}

[[nodiscard]]
inline linal::hmatf to_linal(const glm::dmat4& mat) {
    linal::hmatf result;
    using size_type = linal::hmatf::size_type;
    for (size_type i{0}; i < 4; ++i) {
        for (size_type j{0}; j < 4; ++j) {
            result(i, j) = static_cast<float>(mat[i][j]);
        }
    }
    return result;
}

class Camera {
  public:
    struct PerspectiveParams {
        double fov{30.0};         // Field of view in degrees
        double near_plane{0.01};  // Near clipping plane
        double far_plane{3000.0}; // Far clipping plane

        void validate() const {
            RENDERER_ASSERT(fov > 0.0 && fov < 180.0);
            RENDERER_ASSERT(near_plane > 0.0 && far_plane > near_plane);
        }
    };

    struct OrthographicParams {
        double width{10.0};       // Orthographic view width
        double height{10.0};      // Orthographic view height
        double near_plane{0.01};  // Near clipping plane
        double far_plane{3000.0}; // Far clipping plane

        void validate() const {
            RENDERER_ASSERT(width > 0.0 && height > 0.0);
            RENDERER_ASSERT(near_plane > 0.0 && far_plane > near_plane);
        }
    };

  private:
    // Core camera properties
    Viewport m_viewport;
    glm::dvec3 m_position{};
    glm::dvec3 m_target{};
    glm::dvec3 m_vertical{};
    glm::mat4 m_model{1.0};

    // Projection parameters
    PerspectiveParams m_perspective;
    OrthographicParams m_orthographic;
    CameraProjectionType m_activeProjection{CameraProjectionType::PERSPECTIVE};

    // Cached matrices (computed on demand)
    mutable bool m_viewDirty{true};
    mutable bool m_projectionDirty{true};
    mutable glm::mat4 m_view{1.0};
    mutable glm::mat4 m_projection{1.0};

    // Legacy orthographic bounds (for backward compatibility)
    double m_right{5.0};
    double m_left{-5.0};
    double m_top{5.0};
    double m_bottom{-5.0};

  public:
    Camera() {
        set_position({1.0, 1.0, 1.0});
        set_target({0.0, 0.0, 0.0});
        set_vertical({0.0, 0.0, 1.0});
        set_viewport(0, 0, 800, 600);
    }

    Camera(std::uint32_t width,
           std::uint32_t height,
           const glm::dvec3& position,
           const glm::dvec3& target,
           const glm::dvec3& vertical)
        : m_viewport(0, 0, width, height)
        , m_position(position)
        , m_target(target)
        , m_vertical(vertical) {
        m_perspective.validate();
        set_viewport(0, 0, width, height);
    }

    // Viewport accessors
    [[nodiscard]]
    const Viewport& get_viewport() const {
        return m_viewport;
    }
    [[nodiscard]]
    std::uint32_t get_viewport_xpos() const {
        return m_viewport.get_xpos();
    }
    [[nodiscard]]
    std::uint32_t get_viewport_ypos() const {
        return m_viewport.get_ypos();
    }
    [[nodiscard]]
    std::uint32_t get_viewport_width() const {
        return m_viewport.get_width();
    }
    [[nodiscard]]
    std::uint32_t get_viewport_height() const {
        return m_viewport.get_height();
    }

    // Camera position and orientation
    [[nodiscard]]
    glm::dvec3 get_position() const {
        return m_position;
    }
    [[nodiscard]]
    glm::dvec3 get_target() const {
        return m_target;
    }
    [[nodiscard]]
    glm::dvec3 get_vertical() const {
        return m_vertical;
    }

    // Projection parameters accessors
    [[nodiscard]]
    const PerspectiveParams& get_perspective_params() const {
        return m_perspective;
    }
    [[nodiscard]]
    const OrthographicParams& get_orthographic_params() const {
        return m_orthographic;
    }
    [[nodiscard]]
    CameraProjectionType get_projection_type() const {
        return m_activeProjection;
    }

    // Legacy accessors for backward compatibility
    [[nodiscard]]
    double get_fov() const {
        return m_perspective.fov;
    }
    [[nodiscard]]
    double get_near_plane() const {
        return m_activeProjection == CameraProjectionType::PERSPECTIVE ? m_perspective.near_plane
                                                                       : m_orthographic.near_plane;
    }
    [[nodiscard]]
    double get_far_plane() const {
        return m_activeProjection == CameraProjectionType::PERSPECTIVE ? m_perspective.far_plane
                                                                       : m_orthographic.far_plane;
    }

    // Matrix accessors
    [[nodiscard]]
    glm::mat4 get_model_matrix() const {
        return m_model;
    }
    [[nodiscard]]
    glm::mat4 get_normal_matrix() const {
        return glm::transpose(glm::inverse(glm::mat3(m_model)));
    }
    [[nodiscard]]
    glm::mat4 get_view_matrix() const {
        update_view_matrix();
        return m_view;
    }
    [[nodiscard]]
    glm::mat4 get_perspective_matrix() const {
        update_projection_matrix();
        return m_activeProjection == CameraProjectionType::PERSPECTIVE ? m_projection : compute_perspective_matrix();
    }
    [[nodiscard]]
    glm::mat4 get_ortho_projection_matrix() const {
        update_projection_matrix();
        return m_activeProjection == CameraProjectionType::ORTHOGRAPHIC ? m_projection : compute_orthographic_matrix();
    }
    [[nodiscard]]
    glm::mat4 get_projection_matrix() const {
        update_projection_matrix();
        return m_projection;
    }
    [[nodiscard]]
    glm::mat4 get_proj_mvp() const {
        return get_perspective_matrix() * get_view_matrix() * m_model;
    }
    [[nodiscard]]
    glm::mat4 get_ortho_mvp() const {
        return get_ortho_projection_matrix() * get_view_matrix() * m_model;
    }
    [[nodiscard]]
    glm::mat4 get_mvp() const {
        return get_projection_matrix() * get_view_matrix() * m_model;
    }

    // Camera orientation helpers
    [[nodiscard]]
    glm::dvec3 right() const {
        return glm::normalize(glm::cross(gaze(), get_vertical()));
    }
    [[nodiscard]]
    glm::dvec3 up() const {
        return glm::normalize(glm::cross(right(), gaze()));
    }
    [[nodiscard]]
    glm::dvec3 gaze() const {
        RENDERER_ASSERT(m_target != m_position);
        return glm::normalize(m_target - m_position);
    }

    Camera& set_position(const glm::dvec3& position) {
        RENDERER_ASSERT(position != m_target);
        m_position = position;
        m_viewDirty = true;
        return *this;
    }

    Camera& set_target(const glm::dvec3& target) {
        RENDERER_ASSERT(target != m_position);
        m_target = target;
        m_viewDirty = true;
        return *this;
    }

    Camera& set_vertical(const glm::dvec3& vertical) {
        m_vertical = glm::normalize(vertical);
        m_viewDirty = true;
        return *this;
    }

    Camera& set_projection_type(CameraProjectionType projectionType) {
        if (m_activeProjection != projectionType) {
            m_activeProjection = projectionType;
            m_projectionDirty = true;
        }
        return *this;
    }

    // Perspective parameter setters
    Camera& set_perspective_params(const PerspectiveParams& params) {
        params.validate();
        if (m_perspective.fov != params.fov || m_perspective.near_plane != params.near_plane ||
            m_perspective.far_plane != params.far_plane) {
            m_perspective = params;
            if (m_activeProjection == CameraProjectionType::PERSPECTIVE) {
                m_projectionDirty = true;
            }
        }
        return *this;
    }

    Camera& set_fov(double fov_deg) {
        RENDERER_ASSERT(fov_deg > 0.0 && fov_deg < 180.0);
        if (m_perspective.fov != fov_deg) {
            m_perspective.fov = fov_deg;
            if (m_activeProjection == CameraProjectionType::PERSPECTIVE) {
                m_projectionDirty = true;
            }
        }
        return *this;
    }

    Camera& set_perspective_near_plane(double near_plane) {
        RENDERER_ASSERT(near_plane > 0.0 && near_plane < m_perspective.far_plane);
        if (m_perspective.near_plane != near_plane) {
            m_perspective.near_plane = near_plane;
            if (m_activeProjection == CameraProjectionType::PERSPECTIVE) {
                m_projectionDirty = true;
            }
        }
        return *this;
    }

    Camera& set_perspective_far_plane(double far_plane) {
        RENDERER_ASSERT(far_plane > m_perspective.near_plane);
        if (m_perspective.far_plane != far_plane) {
            m_perspective.far_plane = far_plane;
            if (m_activeProjection == CameraProjectionType::PERSPECTIVE) {
                m_projectionDirty = true;
            }
        }
        return *this;
    }

    // Orthographic parameter setters
    Camera& set_orthographic_params(const OrthographicParams& params) {
        params.validate();
        if (m_orthographic.width != params.width || m_orthographic.height != params.height ||
            m_orthographic.near_plane != params.near_plane || m_orthographic.far_plane != params.far_plane) {
            m_orthographic = params;
            if (m_activeProjection == CameraProjectionType::ORTHOGRAPHIC) {
                m_projectionDirty = true;
            }
        }
        return *this;
    }

    Camera& set_orthographic_size(double width, double height) {
        RENDERER_ASSERT(width > 0.0 && height > 0.0);
        if (m_orthographic.width != width || m_orthographic.height != height) {
            m_orthographic.width = width;
            m_orthographic.height = height;
            if (m_activeProjection == CameraProjectionType::ORTHOGRAPHIC) {
                m_projectionDirty = true;
            }
        }
        return *this;
    }

    Camera& set_orthographic_near_plane(double near_plane) {
        RENDERER_ASSERT(near_plane > 0.0 && near_plane < m_orthographic.far_plane);
        if (m_orthographic.near_plane != near_plane) {
            m_orthographic.near_plane = near_plane;
            if (m_activeProjection == CameraProjectionType::ORTHOGRAPHIC) {
                m_projectionDirty = true;
            }
        }
        return *this;
    }

    Camera& set_orthographic_far_plane(double far_plane) {
        RENDERER_ASSERT(far_plane > m_orthographic.near_plane);
        if (m_orthographic.far_plane != far_plane) {
            m_orthographic.far_plane = far_plane;
            if (m_activeProjection == CameraProjectionType::ORTHOGRAPHIC) {
                m_projectionDirty = true;
            }
        }
        return *this;
    }

    // Legacy setters for backward compatibility
    Camera& set_near_plane(double near_plane) {
        if (m_activeProjection == CameraProjectionType::PERSPECTIVE) {
            set_perspective_near_plane(near_plane);
        } else {
            set_orthographic_near_plane(near_plane);
        }
        return *this;
    }

    Camera& set_far_plane(double far_plane) {
        if (m_activeProjection == CameraProjectionType::PERSPECTIVE) {
            set_perspective_far_plane(far_plane);
        } else {
            set_orthographic_far_plane(far_plane);
        }
        return *this;
    }

    Camera& set_viewport(std::uint32_t xpos, std::uint32_t ypos, std::uint32_t width, std::uint32_t height) {
        m_viewport = Viewport(xpos, ypos, width, height);
        m_projectionDirty = true;
        return *this;
    }

    // Legacy orthographic projection setter (for backward compatibility)
    Camera& set_ortho_projection(double width, double height, double near_plane, double far_plane) {
        OrthographicParams params{width, height, near_plane, far_plane};
        set_orthographic_params(params);

        // Update legacy bounds
        const double halfwidth = width / 2.0 * m_viewport.get_aspect_ratio();
        const double halfheight = height / 2.0;
        m_right = halfwidth;
        m_left = -halfwidth;
        m_top = halfheight;
        m_bottom = -halfheight;

        return *this;
    }

    void zoom_perspective_projection(double amount) {
        const glm::dvec3 pos = get_position() + gaze() * amount;
        set_position(pos);
    }

    void zoom_ortho_projection(double amount) {
        const double zoom = 1.0 + amount; // zoom < 1.0 -> zoom out, zoom > 1.0 -> zoom in
        RENDERER_ASSERT(zoom > 0.0);
        m_orthographic.width = m_orthographic.width / zoom;
        m_orthographic.height = m_orthographic.height / zoom;
        if (m_activeProjection == CameraProjectionType::ORTHOGRAPHIC) {
            m_projectionDirty = true;
        }
    }

    void look_at(const glm::dvec3& position, const glm::dvec3& target, const glm::dvec3& vertical) {
        RENDERER_ASSERT(glm::length(vertical) > 0.0);
        RENDERER_ASSERT(position != target);
        m_position = position;
        m_target = target;
        m_vertical = vertical;
        m_viewDirty = true;
    }

    /** @brief Create a ray from screen coordinates using the current projection type.
     *
     * @param screenX Screen X coordinate (in pixels)
     * @param screenY Screen Y coordinate (in pixels)
     * @return PickRay from camera through the specified screen point
     */
    [[nodiscard]]
    PickRay get_ray(double screenX, double screenY) const {
        switch (m_activeProjection) {
        case CameraProjectionType::PERSPECTIVE:  return create_perspective_ray(screenX, screenY);
        case CameraProjectionType::ORTHOGRAPHIC: return create_orthographic_ray(screenX, screenY);
        default:                                 RENDERER_ASSERT(false); return {};
        }
    }

    /** @brief Create a ray from normalized device coordinates using current projection.
     *
     * @param ndcX Normalized device coordinate X [-1, +1]
     * @param ndcY Normalized device coordinate Y [-1, +1]
     * @return PickRay from camera through the specified NDC point
     */
    [[nodiscard]]
    PickRay get_ray_from_ndc(double ndcX, double ndcY) const {
        // Convert NDC back to screen coordinates
        const double screenX = (ndcX + 1.0) * static_cast<double>(m_viewport.get_width()) / 2.0;
        const double screenY = (ndcY + 1.0) * static_cast<double>(m_viewport.get_height()) / 2.0;
        return get_ray(screenX, screenY);
    }

    /** @brief Create a ray from the center of the viewport.
     *
     * @return PickRay from camera through the center of the screen
     */
    [[nodiscard]]
    PickRay get_center_ray() const {
        const double centerX = static_cast<double>(m_viewport.get_width()) / 2.0;
        const double centerY = static_cast<double>(m_viewport.get_height()) / 2.0;
        return get_ray(centerX, centerY);
    }

    /** @brief Create multiple rays for a screen region (useful for area picking).
     *
     * @param startX Starting screen X coordinate
     * @param startY Starting screen Y coordinate
     * @param endX Ending screen X coordinate
     * @param endY Ending screen Y coordinate
     * @param samplesX Number of samples in X direction
     * @param samplesY Number of samples in Y direction
     * @return Vector of PickRays covering the specified region
     */
    [[nodiscard]]
    std::vector<PickRay> get_rays_for_region(double startX,
                                             double startY,
                                             double endX,
                                             double endY,
                                             std::uint32_t samplesX = 3,
                                             std::uint32_t samplesY = 3) const {
        std::vector<PickRay> rays;
        rays.reserve(samplesX * samplesY);

        for (std::uint32_t y = 0; y < samplesY; ++y) {
            for (std::uint32_t x = 0; x < samplesX; ++x) {
                const double t_x = samplesX > 1 ? static_cast<double>(x) / (samplesX - 1) : 0.5;
                const double t_y = samplesY > 1 ? static_cast<double>(y) / (samplesY - 1) : 0.5;

                const double screenX = startX + t_x * (endX - startX);
                const double screenY = startY + t_y * (endY - startY);

                rays.push_back(get_ray(screenX, screenY));
            }
        }

        return rays;
    }

    /** @brief Create a perspective projection ray from screen coordinates.
     *
     * @param screenX Screen X coordinate (in pixels)
     * @param screenY Screen Y coordinate (in pixels)
     * @return PickRay using perspective projection
     */
    [[nodiscard]]
    PickRay create_perspective_ray(double screenX, double screenY) const;

    /** @brief Create an orthographic projection ray from screen coordinates.
     *
     * @param screenX Screen X coordinate (in pixels)
     * @param screenY Screen Y coordinate (in pixels)
     * @return PickRay using orthographic projection
     */
    [[nodiscard]]
    PickRay create_orthographic_ray(double screenX, double screenY) const;

    /** @brief Check if screen coordinates are within the viewport.
     *
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     * @return True if coordinates are within viewport bounds
     */
    [[nodiscard]]
    bool is_point_in_viewport(double screenX, double screenY) const {
        return screenX >= 0 && screenX < static_cast<double>(m_viewport.get_width()) && screenY >= 0 &&
               screenY < static_cast<double>(m_viewport.get_height());
    }

    /** @brief Convert screen coordinates to normalized device coordinates.
     *
     * @param screenX Screen X coordinate (in pixels)
     * @param screenY Screen Y coordinate (in pixels)
     * @return std::pair<double, double> NDC coordinates (x, y) in range [-1, +1]
     */
    [[nodiscard]]
    std::pair<double, double> screen_to_ndc(double screenX, double screenY) const {
        const double ndcX = 2.0 * screenX / static_cast<double>(m_viewport.get_width()) - 1.0;
        const double ndcY = 2.0 * screenY / static_cast<double>(m_viewport.get_height()) - 1.0;
        return {ndcX, ndcY};
    }

    /** @brief Convert normalized device coordinates to screen coordinates.
     *
     * @param ndcX NDC X coordinate [-1, +1]
     * @param ndcY NDC Y coordinate [-1, +1]
     * @return std::pair<double, double> Screen coordinates (x, y) in pixels
     */
    [[nodiscard]]
    std::pair<double, double> ndc_to_screen(double ndcX, double ndcY) const {
        const double screenX = (ndcX + 1.0) * static_cast<double>(m_viewport.get_width()) / 2.0;
        const double screenY = (ndcY + 1.0) * static_cast<double>(m_viewport.get_height()) / 2.0;
        return {screenX, screenY};
    }

  private:
    void update_view_matrix() const {
        if (m_viewDirty) {
            m_view = glm::lookAt(m_position, m_target, m_vertical);
            m_viewDirty = false;
        }
    }

    void update_projection_matrix() const {
        if (m_projectionDirty) {
            switch (m_activeProjection) {
            case CameraProjectionType::PERSPECTIVE:  m_projection = compute_perspective_matrix(); break;
            case CameraProjectionType::ORTHOGRAPHIC: m_projection = compute_orthographic_matrix(); break;
            default:                                 RENDERER_ASSERT(false); break;
            }
            m_projectionDirty = false;
        }
    }

    [[nodiscard]]
    glm::mat4 compute_perspective_matrix() const {
        return glm::perspective(glm::radians(m_perspective.fov),
                                m_viewport.get_aspect_ratio(),
                                m_perspective.near_plane,
                                m_perspective.far_plane);
    }

    [[nodiscard]]
    glm::mat4 compute_orthographic_matrix() const {
        const double halfwidth = m_orthographic.width / 2.0 * m_viewport.get_aspect_ratio();
        const double halfheight = m_orthographic.height / 2.0;
        return glm::ortho(-halfwidth,
                          halfwidth,
                          -halfheight,
                          halfheight,
                          m_orthographic.near_plane,
                          m_orthographic.far_plane);
    }
};

} // namespace renderer

#endif // RENDERER_CAMERA_HPP
