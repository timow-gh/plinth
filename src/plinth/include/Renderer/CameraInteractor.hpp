#ifndef RENDERER_CAMERAINTERACTOR_HPP
#define RENDERER_CAMERAINTERACTOR_HPP

#include <Renderer/Camera.hpp>
#include <Renderer/CameraAutoFit.hpp>
#include <Renderer/CameraProjectionType.hpp>
#include <Renderer/InputState.hpp>
#include <Renderer/PickRay.hpp>
#include <Renderer/Plane.hpp>
#include <Renderer/RayPlaneIntersection.hpp>
#include <Renderer/Warnings.hpp>
#include <algorithm>
RENDERER_DISABLE_ALL_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/quaternion_double.hpp>
#include <glm/gtx/quaternion.hpp>
RENDERER_ENABLE_ALL_WARNINGS
#include <iostream>
#include <linal/linal.hpp>

namespace renderer {

struct CameraSettings {
    Camera m_camera;
    glm::mat4 m_currentMVP{}; /**< Precalculated the current model view projection matrix. */
    CameraProjectionType m_projectionType{
        CameraProjectionType::PERSPECTIVE}; /**< Whether to use projection matrix or ortho matrix. */
    Plane m_groundPlane{linal::double3{0.0, 0.0, 0.0},
                        linal::double3{0.0, 0.0, 1.0}}; /**< The pan and rotate default plane. */

    double m_zoomPerspFactor{0.2};  /**< The zoom factor used to translate the camera in perspective mode. */
    double m_zoomOrthoFactor{0.05}; /**< The zoom factor used to translate the camera in ortho mode. */

    bool m_isPanStart{false};
    linal::double3 m_panStartFar{0.0};          /**< The far vector at the start of panning. */
    linal::double3 m_panSceneStart{0.0};        /**< The scene vector at the start of panning. */
    linal::double3 m_panCamaraStartPos{0.0};    /**< The camera position at the start of panning. */
    linal::double3 m_panCameraTargetStart{0.0}; /**< The camera target at the start of panning. */

    bool m_isRotateStart{false};
    linal::double3 m_pivot{0.0};           /**< The pivot point of rotation. */
    glm::dvec2 m_rotateScreenXYStart{0.0}; /**< The screen vector at the start of rotation. */
    double m_rotateSpeed{0.2};             /**< The rotation speed. */

    linal::double3 m_defaultPosition{80.0, 80.0, 80.0};
    linal::double3 m_defaultTarget{0.0};
    linal::double3 m_defaultUp{0.0, 0.0, 1.0};
};

class CameraInteractor : private CameraSettings {
    enum class CameraMode {
        NO_MODE = 0,
        ORBIT,
        PAN
    };

    InputState* m_inputState{nullptr};
    CameraMode m_cameraMode{CameraMode::NO_MODE};
    double m_prevXPos{std::numeric_limits<double>::quiet_NaN()};
    double m_prevYPos{std::numeric_limits<double>::quiet_NaN()};
    bool m_wasBlocking{false}; /**< The camera is only blocking when the last event for the  camera
                                  manipulated the position. */

  public:
    CameraInteractor() = default;
    CameraInteractor(const CameraInteractor&) = delete;
    CameraInteractor& operator=(const CameraInteractor&) = delete;
    CameraInteractor(CameraInteractor&& other) noexcept
        : CameraSettings(std::move(other))
        , m_inputState(std::exchange(other.m_inputState, nullptr))
        , m_cameraMode(std::exchange(other.m_cameraMode, CameraMode::NO_MODE)) {}

    CameraInteractor& operator=(CameraInteractor&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        CameraSettings::operator=(std::move(other));
        m_inputState = std::exchange(other.m_inputState, nullptr);
        m_cameraMode = std::exchange(other.m_cameraMode, CameraMode::NO_MODE);
        return *this;
    }
    ~CameraInteractor() = default;

    CameraInteractor(InputState& intputState, const CameraSettings& cameraSettings)
        : CameraSettings(cameraSettings)
        , m_inputState(&intputState) {
        look_at(m_defaultPosition, m_defaultTarget, m_defaultUp);
    }

    void set_near_plane(double nearPlane) {
        m_camera.set_near_plane(nearPlane);
        update_mvp();
    }

    void set_far_plane(double farPlane) {
        m_camera.set_far_plane(farPlane);
        update_mvp();
    }

    [[nodiscard]]
    bool get_was_blocking() const {
        return m_wasBlocking;
    }
    void reset_was_blocking() { m_wasBlocking = false; }

    /** @brief Set the camera position and gaze target.
     *
     * @param position The camera position.
     * @param target The gaze target.
     * @param vertical The up vector.
     */
    void look_at(const linal::double3& position, const linal::double3& target, const linal::double3& vertical) {
        m_camera.look_at(to_glm(position), to_glm(target), to_glm(vertical));
        update_mvp();
    }

    [[nodiscard]]
    linal::double3 get_position() const {
        return to_linal(m_camera.get_position());
    }
    [[nodiscard]]
    linal::double3 get_target() const {
        return to_linal(m_camera.get_target());
    }
    [[nodiscard]]
    linal::double3 get_vertical() const {
        return to_linal(m_camera.get_vertical());
    }
    [[nodiscard]]
    CameraProjectionType get_projection_type() const {
        return m_projectionType;
    }
    [[nodiscard]]
    double get_fov() const {
        return m_camera.get_fov();
    }
    [[nodiscard]]
    double get_near_plane() const {
        return m_camera.get_near_plane();
    }
    [[nodiscard]]
    double get_far_plane() const {
        return m_camera.get_far_plane();
    }
    [[nodiscard]]
    Camera::OrthographicParams get_orthographic_params() const {
        return m_camera.get_orthographic_params();
    }

    [[nodiscard]]
    linal::double3 get_default_position() const {
        return m_defaultPosition;
    }
    [[nodiscard]]
    linal::double3 get_default_target() const {
        return m_defaultTarget;
    }
    [[nodiscard]]
    linal::double3 get_default_up() const {
        return m_defaultUp;
    }

    void set_zoom_factor(double zoomFactor) {

        switch (m_projectionType) {
        case CameraProjectionType::PERSPECTIVE:  m_zoomPerspFactor = zoomFactor; break;
        case CameraProjectionType::ORTHOGRAPHIC: m_zoomOrthoFactor = zoomFactor; break;
        }
        update_mvp();
    }

    /** @brief Set the projection type of the camera.
     *
     * @param projectionType Currently either perspective or orthographic.
     */
    void set_projection_type(CameraProjectionType projectionType) {
        m_projectionType = projectionType;
        m_camera.set_projection_type(projectionType);
        update_mvp();
    }

    void set_ground_plane(const Plane& groundPlane) { m_groundPlane = groundPlane; }

    void set_viewport(std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height) {
        m_camera.set_viewport(x, y, width, height);
        update_mvp();
    }

    void apply_auto_fit_result(const CameraAutoFitResult& result) {
        m_camera.look_at(to_glm(result.position), to_glm(result.target), to_glm(result.vertical));
        if (m_projectionType == CameraProjectionType::ORTHOGRAPHIC) {
            m_camera.set_orthographic_size(result.orthographicWidth, result.orthographicHeight);
        }
        m_camera.set_far_plane(result.farPlane);
        update_mvp();
    }

    void on_scroll([[maybe_unused]] double xoffset, double yoffset) {
        linal::double3 groundPlaneIntersection;
        linal::double3 pos = to_linal(m_camera.get_position());

        auto pickRay = get_pick_ray(m_inputState->cursorPosState.xpos, m_inputState->cursorPosState.ypos);
        if (!ray_plane_intersection(pos, pickRay.direction, m_groundPlane, groundPlaneIntersection)) {
            return;
        }
        double distance = linal::length(pos - groundPlaneIntersection);
        double distanceZoomFactor = distance / 10;
        distanceZoomFactor = std::clamp(distanceZoomFactor, 1.0, 100.0);
        switch (m_projectionType) {
        case CameraProjectionType::PERSPECTIVE: {
            m_camera.zoom_perspective_projection(yoffset * m_zoomPerspFactor * distanceZoomFactor);
            break;
        }
        case CameraProjectionType::ORTHOGRAPHIC: {
            m_camera.zoom_ortho_projection(yoffset * m_zoomOrthoFactor);
            break;
        }
        default: {
            RENDERER_ASSERT(false);
            return;
        }
        }

        m_wasBlocking = true;
        update_mvp();
    }

    void on_cursor_position(double xpos, double ypos) {
        ypos = m_camera.get_viewport().get_height() - ypos;

        if (m_prevXPos == std::numeric_limits<double>::quiet_NaN() ||
            m_prevYPos == std::numeric_limits<double>::quiet_NaN()) {
            m_prevXPos = xpos;
            m_prevYPos = ypos;
            return;
        }

        const double deltaPrevXPos = xpos - m_prevXPos;
        const double deltaPrevYPos = ypos - m_prevYPos;

        m_prevXPos = xpos;
        m_prevYPos = ypos;

        const double width = m_camera.get_viewport().get_width();
        const double height = m_camera.get_viewport().get_height();
        const double onePixelWidth = 1.0 / width;
        const double onePixelHeight = 1.0 / height;

        if (std::abs(deltaPrevXPos) < onePixelWidth && std::abs(deltaPrevYPos) < onePixelHeight) {
            return;
        }

        if (m_cameraMode == CameraMode::PAN) {
            m_wasBlocking = true;

            // Convert mouse delta to normalized device coordinates
            const double ndcDeltaX = deltaPrevXPos / width * 2.0;
            const double ndcDeltaY = deltaPrevYPos / height * 2.0;

            // Get camera vectors
            const linal::double3 cameraPos = to_linal(m_camera.get_position());
            const linal::double3 cameraTarget = to_linal(m_camera.get_target());
            const linal::double3 cameraUp = to_linal(m_camera.get_vertical());
            const linal::double3 cameraForward = linal::normalize(cameraTarget - cameraPos);
            const linal::double3 cameraRight = linal::normalize(linal::cross(cameraForward, cameraUp));
            const linal::double3 cameraUpNorm = linal::normalize(cameraUp);

            // Calculate pan distance based on camera-to-target distance
            const double cameraToTargetDistance = linal::length(cameraTarget - cameraPos);

            double panSpeedFactor = 1.0;
            if (m_projectionType == CameraProjectionType::PERSPECTIVE) {
                const double fov = m_camera.get_fov();
                panSpeedFactor = cameraToTargetDistance * std::tan(glm::radians(fov * 0.5));
            } else {
                // Use orthographic viewport size
                const auto orthParams = m_camera.get_orthographic_params();
                panSpeedFactor = orthParams.width * 0.5;
            }

            const linal::double3 translation =
                cameraRight * (-ndcDeltaX * panSpeedFactor) + cameraUpNorm * (-ndcDeltaY * panSpeedFactor);

            // Apply translation to both camera position and target
            const linal::double3 newCameraPos = cameraPos + translation;
            const linal::double3 newCameraTarget = cameraTarget + translation;

            m_camera.look_at(to_glm(newCameraPos), to_glm(newCameraTarget), to_glm(cameraUpNorm));
            update_mvp();
        }

        if (m_cameraMode == CameraMode::ORBIT) {
            m_wasBlocking = true;

            if (m_isRotateStart) {
                m_isRotateStart = false;
                linal::double3 pos = to_linal(m_camera.get_position());
                linal::double3 gaze = to_linal(m_camera.gaze());
                if (!ray_plane_intersection(pos, gaze, m_groundPlane, m_pivot)) {
                    m_isRotateStart = true;
                    m_wasBlocking = false;
                    return;
                }
                m_rotateScreenXYStart = glm::vec2{xpos, ypos};
            }

            double deltaXPos = m_rotateScreenXYStart[0] - xpos;
            double deltaYPos = m_rotateScreenXYStart[1] - ypos;

            m_rotateScreenXYStart = glm::vec2{xpos, ypos};

            constexpr double epsilon = 0.00001;
            if (std::abs(deltaXPos) < epsilon) {
                deltaXPos = 0.0;
            }

            if (std::abs(deltaYPos) < epsilon) {
                deltaYPos = 0.0;
            }

            if (deltaXPos == 0.0 && deltaYPos == 0.0) {
                m_wasBlocking = false;
                return;
            }

            auto vertical = m_camera.get_vertical();
            auto right = m_camera.right();
            const glm::dquat zQuat = glm::angleAxis(glm::radians(deltaXPos * m_rotateSpeed),
                                                    glm::dvec3{vertical[0], vertical[1], vertical[2]});
            const glm::dquat rightQuat =
                glm::angleAxis(glm::radians(-deltaYPos * m_rotateSpeed), glm::dvec3{right[0], right[1], right[2]});
            const glm::dquat rotQuat = zQuat * rightQuat;

            glm::dvec3 position = m_camera.get_position();
            glm::dvec3 pivot = to_glm(m_pivot);
            glm::dvec3 pivotToEye = position - pivot;
            const double pivotToEyeDist = glm::length(pivotToEye);
            pivotToEye = glm::normalize(pivotToEye);
            const glm::dvec3 newPivotToEye = rotQuat * pivotToEye;
            const glm::dvec3 newEye = pivot + glm::normalize(newPivotToEye) * pivotToEyeDist;

            m_camera.look_at(newEye, pivot, m_camera.get_vertical());
            update_mvp();
        }
    }

    void on_mouse_button(int button, Action action, [[maybe_unused]] Mods mods) {
        if (action == Action::PRESS && m_cameraMode == CameraMode::NO_MODE) {
            switch (button) {
            case GLFW_MOUSE_BUTTON_RIGHT: {
                m_cameraMode = CameraMode::ORBIT;
                m_isRotateStart = true;
                return;
            }
            case GLFW_MOUSE_BUTTON_MIDDLE: {
                m_cameraMode = CameraMode::PAN;
                m_isPanStart = true;
                return;
            }
            default:
                m_cameraMode = CameraMode::NO_MODE;
                m_wasBlocking = false;
                return;
            }
        } else if (action == Action::RELEASE) {
            m_cameraMode = CameraMode::NO_MODE;
            return;
        }
    }

    void on_framebuffer_size(std::uint32_t width, std::uint32_t height) {
        if (width == 0 || height == 0) {
            return;
        }

        m_camera.set_viewport(0, 0, width, height);
        update_mvp();
    }

    [[nodiscard]]
    PickRay get_pick_ray(double xpos, double ypos) const {
        switch (m_projectionType) {
        case CameraProjectionType::PERSPECTIVE:  return m_camera.create_perspective_ray(xpos, ypos);
        case CameraProjectionType::ORTHOGRAPHIC: return m_camera.create_orthographic_ray(xpos, ypos);
        default:                                 {
            RENDERER_ASSERT(false);
            return {};
        }
        }
    }

    [[nodiscard]]
    const Viewport& get_viewport() const {
        return m_camera.get_viewport();
    }

    [[nodiscard]]
    linal::hmatf get_model_matrix() const {
        return to_hmatf(m_camera.get_model_matrix());
    }
    [[nodiscard]]
    linal::hmatf get_view_matrix() const {
        return to_hmatf(m_camera.get_view_matrix());
    }
    [[nodiscard]]
    linal::hmatf get_projection_matrix() const {
        return get_projection_matrix_impl();
    }
    [[nodiscard]]
    linal::hmatf get_current_MVP() const {
        return to_hmatf(m_currentMVP);
    }
    [[nodiscard]]
    linal::hmatf get_normal_matrix() const {
        return to_hmatf(m_camera.get_normal_matrix());
    }

  private:
    [[nodiscard]]
    static linal::hmatf to_hmatf(const glm::dmat4& mat) {
        linal::hmatf result;
        using size_type = linal::hmatf::size_type;
        for (size_type i{0}; i < 4; ++i) {
            for (size_type j{0}; j < 4; ++j) {
                result(i, j) = static_cast<float>(mat[i][j]);
            }
        }
        return result;
    }

    [[nodiscard]]
    linal::hmatf get_projection_matrix_impl() const {
        switch (m_projectionType) {
        case CameraProjectionType::PERSPECTIVE:  return to_hmatf(m_camera.get_perspective_matrix());
        case CameraProjectionType::ORTHOGRAPHIC: return to_hmatf(m_camera.get_ortho_projection_matrix());
        default:                                 {
            RENDERER_ASSERT(false);
            return {};
        }
        }
    }

    void update_mvp() {

        switch (m_projectionType) {
        case CameraProjectionType::PERSPECTIVE:  m_currentMVP = m_camera.get_proj_mvp(); break;
        case CameraProjectionType::ORTHOGRAPHIC: m_currentMVP = m_camera.get_ortho_mvp(); break;
        default:                                 {
            RENDERER_ASSERT(false);
            return;
        }
        }
    }
};

} // namespace renderer

#endif // RENDERER_CAMERAINTERACTOR_HPP
