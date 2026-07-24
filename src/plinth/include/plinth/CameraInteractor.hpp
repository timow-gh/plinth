#ifndef RENDERER_CAMERAINTERACTOR_HPP
#define RENDERER_CAMERAINTERACTOR_HPP

#include <plinth/Camera.hpp>
#include <plinth/CameraAutoFit.hpp>
#include <plinth/CameraPivotMode.hpp>
#include <plinth/CameraProjectionType.hpp>
#include <plinth/InputState.hpp>
#include <plinth/PickRay.hpp>
#include <plinth/Plane.hpp>
#include <plinth/RayPlaneIntersection.hpp>
#include <plinth/Warnings.hpp>
#include <algorithm>
#include <cmath>
#include <functional>
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

    bool m_isRotateStart{false};
    linal::double3 m_pivot{0.0};           /**< The pivot point of rotation. */
    glm::dvec2 m_rotateScreenXYStart{0.0}; /**< The screen vector at the start of rotation. */
    double m_rotateSpeed{0.2};             /**< The rotation speed. */

    linal::double3 m_defaultPosition{80.0, 80.0, 80.0};
    linal::double3 m_defaultTarget{0.0};
    linal::double3 m_defaultUp{0.0, 0.0, 1.0};
};

/** @brief Named camera view presets (front/back/left/right/top/bottom/iso). */
enum class PresetView {
    FRONT,
    BACK,
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
    ISO
};

struct PresetViewDirection {
    linal::double3 direction; /**< Unit vector, target -> camera. */
    linal::double3 up;        /**< Up vector to use for this preset. */
};

/** @brief Compute the unit direction (target -> camera) and up vector for a preset view.
 *
 * Follows the same Z-up convention as CameraSettings::m_defaultUp.
 */
[[nodiscard]] inline PresetViewDirection preset_view_direction(PresetView view) {
    switch (view) {
    case PresetView::FRONT: return {{0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}};
    case PresetView::BACK:  return {{0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    case PresetView::LEFT:  return {{-1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    case PresetView::RIGHT: return {{1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    // TOP/BOTTOM: gaze is parallel to the default Z-up vector, so the up vector must be a
    // different axis (Y) here or Camera::right()/up() degenerates (cross product of parallel vectors).
    case PresetView::TOP:    return {{0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}};
    case PresetView::BOTTOM: return {{0.0, 0.0, -1.0}, {0.0, 1.0, 0.0}};
    case PresetView::ISO:    return {linal::normalize(linal::double3{1.0, -1.0, 1.0}), {0.0, 0.0, 1.0}};
    }
    RENDERER_ASSERT(false);
    return {{0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}};
}

class CameraInteractor : private CameraSettings {
    enum class CameraMode {
        NO_MODE = 0,
        ORBIT,
        PAN
    };

  public:
    enum class NavigationStyle {
        ORBIT, // existing behavior: right-drag orbits around a ground-plane pivot, middle-drag pans
        FLY    // new: WASD(+QE) translates the camera, right-drag mouse-looks in place
    };

    using KeyPressQuery = std::function<bool(Key)>;

    /** @brief Alias for the free-standing renderer::PresetView, exposed as a nested name for callers
     *  that prefer to write CameraInteractor::PresetView. */
    using PresetView = renderer::PresetView;

    /** @brief Alias for the free-standing renderer::CameraPivotMode, exposed as a nested name for
     *  callers that prefer to write CameraInteractor::PivotMode. */
    using PivotMode = renderer::CameraPivotMode;

  private:
    InputState* m_inputState{nullptr};
    CameraMode m_cameraMode{CameraMode::NO_MODE};
    double m_prevXPos{std::numeric_limits<double>::quiet_NaN()};
    double m_prevYPos{std::numeric_limits<double>::quiet_NaN()};
    bool m_wasBlocking{false}; /**< The camera is only blocking when the last event for the  camera
                                  manipulated the position. */

    NavigationStyle m_navigationStyle{NavigationStyle::ORBIT};
    PivotMode m_pivotMode{PivotMode::GROUND_CURSOR};
    double m_flySpeed{5.0}; // world units per second

    bool m_isTransitioning{false};
    double m_transitionElapsedSeconds{0.0};
    double m_transitionDurationSeconds{0.4}; // default transition duration, in seconds
    linal::double3 m_transitionStartTarget{0.0};
    linal::double3 m_transitionEndTarget{0.0};
    linal::double3 m_transitionStartDirection{0.0};
    linal::double3 m_transitionEndDirection{0.0};
    double m_transitionStartDistance{0.0};
    double m_transitionEndDistance{0.0};
    linal::double3 m_transitionStartUp{0.0};
    linal::double3 m_transitionEndUp{0.0};

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

    void set_orthographic_size(double width, double height) {
        m_camera.set_orthographic_size(width, height);
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

    void set_navigation_style(NavigationStyle style) noexcept { m_navigationStyle = style; }
    [[nodiscard]]
    NavigationStyle get_navigation_style() const noexcept {
        return m_navigationStyle;
    }
    void set_pivot_mode(PivotMode mode) noexcept { m_pivotMode = mode; }
    [[nodiscard]]
    PivotMode get_pivot_mode() const noexcept {
        return m_pivotMode;
    }
    void set_fly_speed(double unitsPerSecond) noexcept { m_flySpeed = unitsPerSecond; }
    [[nodiscard]]
    double get_fly_speed() const noexcept {
        return m_flySpeed;
    }

    /** @brief Smoothly transition the camera to a named preset view (front/top/iso/...).
     *
     * The pivot (current gaze target) and camera-to-target distance are preserved; only the
     * viewing direction and up vector animate. If the transition duration is 0 (or negative),
     * the camera snaps to the preset view instantly instead of animating.
     */
    void go_to_preset_view(PresetView view) {
        const linal::double3 pos = to_linal(m_camera.get_position());
        const linal::double3 target = to_linal(m_camera.get_target());
        const double distance = linal::length(pos - target);
        RENDERER_ASSERT(distance > 1.0e-9);

        const PresetViewDirection preset = preset_view_direction(view);
        begin_pose_transition(target + preset.direction * distance, target, preset.up);
    }

    /** @brief Smoothly transition the camera to an arbitrary destination pose (position/target/up),
     *  e.g. a geometry-fit result. Unlike go_to_preset_view, this does not assume the pivot or
     *  distance stay fixed - both may change if endTarget/endPosition differ from the current pose.
     */
    void transition_to_pose(const linal::double3& endPosition,
                            const linal::double3& endTarget,
                            const linal::double3& endUp) {
        begin_pose_transition(endPosition, endTarget, endUp);
    }

    void set_view_transition_duration(double seconds) noexcept { m_transitionDurationSeconds = std::max(0.0, seconds); }
    [[nodiscard]]
    bool is_transitioning_view() const noexcept {
        return m_isTransitioning;
    }

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

    /** @brief Advance per-frame camera state: fly-mode keyboard navigation and any in-flight
     *  preset-view transition.
     *
     * The fly-movement portion is a no-op unless the navigation style is FLY and deltaSeconds is
     * positive. The preset-view transition-advance portion runs regardless of navigation style
     * (guarded internally by whether a transition is in flight) so a transition started while in
     * ORBIT style can still play out.
     *
     * @param deltaSeconds Time elapsed since the previous frame, in seconds.
     * @param isKeyPressed Query function returning whether a given key is currently held down.
     */
    void update(double deltaSeconds, const KeyPressQuery& isKeyPressed) {
        if (m_navigationStyle == NavigationStyle::FLY && deltaSeconds > 0.0) {
            const linal::double3 cameraPos = to_linal(m_camera.get_position());
            const linal::double3 cameraTarget = to_linal(m_camera.get_target());
            const linal::double3 forward = linal::normalize(cameraTarget - cameraPos);
            const linal::double3 upVec = to_linal(m_camera.get_vertical());
            const linal::double3 right = linal::normalize(linal::cross(forward, upVec));

            linal::double3 move{0.0, 0.0, 0.0};
            if (isKeyPressed(Key::KEY_W)) { move = move + forward; }
            if (isKeyPressed(Key::KEY_S)) { move = move - forward; }
            if (isKeyPressed(Key::KEY_D)) { move = move + right; }
            if (isKeyPressed(Key::KEY_A)) { move = move - right; }
            if (isKeyPressed(Key::KEY_E)) { move = move + upVec; }
            if (isKeyPressed(Key::KEY_Q)) { move = move - upVec; }

            if (linal::length(move) >= 1.0e-12) {
                const linal::double3 translation = linal::normalize(move) * (m_flySpeed * deltaSeconds);
                m_camera.look_at(to_glm(cameraPos + translation), to_glm(cameraTarget + translation), to_glm(upVec));
                m_wasBlocking = true;
                update_mvp();
            }
        }

        if (m_isTransitioning) {
            m_transitionElapsedSeconds += std::max(0.0, deltaSeconds);
            double t = m_transitionDurationSeconds > 0.0 ? m_transitionElapsedSeconds / m_transitionDurationSeconds
                                                          : 1.0;
            t = std::clamp(t, 0.0, 1.0);
            const double easedT = t * t * (3.0 - 2.0 * t); // smoothstep

            // Slerp the direction vector (not a plain lerp) so distance-to-target stays constant
            // throughout the transition instead of the camera cutting through the pivot on the
            // short path.
            const double cosAngle =
                std::clamp(linal::dot(m_transitionStartDirection, m_transitionEndDirection), -1.0, 1.0);
            const double angle = std::acos(cosAngle);
            linal::double3 direction;
            if (angle < 1.0e-6) {
                direction = m_transitionEndDirection;
            } else {
                linal::double3 axis = linal::cross(m_transitionStartDirection, m_transitionEndDirection);
                if (linal::length(axis) < 1.0e-9) {
                    // Start/end directions are antiparallel (e.g. FRONT<->BACK, LEFT<->RIGHT,
                    // TOP<->BOTTOM): their cross product is the exact zero vector, which would
                    // normalize to NaN. Rotate around an arbitrary axis perpendicular to the start
                    // direction instead - any such axis produces a valid 180-degree turn.
                    const linal::double3 arbitrary = std::abs(m_transitionStartDirection[0]) < 0.9
                                                          ? linal::double3{1.0, 0.0, 0.0}
                                                          : linal::double3{0.0, 1.0, 0.0};
                    axis = linal::cross(m_transitionStartDirection, arbitrary);
                }
                axis = linal::normalize(axis);
                const glm::dquat rot = glm::angleAxis(angle * easedT, glm::dvec3{axis[0], axis[1], axis[2]});
                const glm::dvec3 rotated =
                    rot * glm::dvec3{m_transitionStartDirection[0],
                                      m_transitionStartDirection[1],
                                      m_transitionStartDirection[2]};
                direction = linal::normalize(to_linal(rotated));
            }
            const linal::double3 up =
                linal::normalize(m_transitionStartUp * (1.0 - easedT) + m_transitionEndUp * easedT);
            const linal::double3 target =
                m_transitionStartTarget * (1.0 - easedT) + m_transitionEndTarget * easedT;
            const double distanceValue =
                m_transitionStartDistance * (1.0 - easedT) + m_transitionEndDistance * easedT;
            const linal::double3 position = target + direction * distanceValue;

            m_camera.look_at(to_glm(position), to_glm(target), to_glm(up));
            update_mvp();

            if (t >= 1.0) {
                m_isTransitioning = false;
            }
        }
    }

    void on_cursor_position(double xpos, double ypos) {
        ypos = m_camera.get_viewport().get_height() - ypos;

        if (std::isnan(m_prevXPos) || std::isnan(m_prevYPos)) {
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

        if (m_cameraMode == CameraMode::ORBIT && m_navigationStyle == NavigationStyle::FLY) {
            m_wasBlocking = true;

            // Fly mode mouse-look always works, regardless of the ground plane, so just re-arm the
            // screen-space delta tracking the same way orbit mode does on the first drag event.
            if (m_isRotateStart) {
                m_isRotateStart = false;
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
            const glm::dvec3 newForward = rotQuat * (m_camera.get_target() - position);
            m_camera.look_at(position, position + newForward, m_camera.get_vertical());
            update_mvp();
            return;
        }

        if (m_cameraMode == CameraMode::ORBIT) {
            m_wasBlocking = true;

            if (m_isRotateStart) {
                m_isRotateStart = false;
                if (m_pivotMode == PivotMode::ORIGIN) {
                    m_pivot = linal::double3{0.0, 0.0, 0.0};
                } else {
                    // Pivot around where the ray through the mouse cursor hits the ground plane,
                    // captured once at drag start. If the cursor ray misses the ground plane, block
                    // rotation and re-arm so it can be retried on the next event.
                    linal::double3 pos = to_linal(m_camera.get_position());
                    const PickRay ray = get_pick_ray(m_inputState->cursorPosState.xpos,
                                                     m_inputState->cursorPosState.ypos);
                    if (!ray_plane_intersection(pos, ray.direction, m_groundPlane, m_pivot)) {
                        m_isRotateStart = true;
                        m_wasBlocking = false;
                        return;
                    }
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

            const glm::dvec3 pivot = to_glm(m_pivot);
            const glm::dvec3 position = m_camera.get_position();
            const glm::dvec3 target = m_camera.get_target();
            const glm::dvec3 newEye = pivot + rotQuat * (position - pivot);
            const glm::dvec3 newTarget = pivot + rotQuat * (target - pivot);

            m_camera.look_at(newEye, newTarget, m_camera.get_vertical());
            update_mvp();
        }
    }

    void on_mouse_button(int button, Action action, [[maybe_unused]] Mods mods) {
        if (action == Action::PRESS && m_cameraMode == CameraMode::NO_MODE) {
            switch (button) {
            case 1: { // GLFW_MOUSE_BUTTON_RIGHT
                m_cameraMode = CameraMode::ORBIT;
                m_isRotateStart = true;
                return;
            }
            case 2: { // GLFW_MOUSE_BUTTON_MIDDLE
                m_cameraMode = CameraMode::PAN;
                return;
            }
            default:
                m_cameraMode = CameraMode::NO_MODE;
                m_wasBlocking = false;
                return;
            }
        } else if (action == Action::RELEASE) {
            // End the gesture only when the button that started it comes up;
            // releasing an unrelated button mid-gesture must not cancel it.
            if ((button == 1 && m_cameraMode == CameraMode::ORBIT) || // GLFW_MOUSE_BUTTON_RIGHT
                (button == 2 && m_cameraMode == CameraMode::PAN)) {   // GLFW_MOUSE_BUTTON_MIDDLE
                m_cameraMode = CameraMode::NO_MODE;
            }
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
    // Begin an animated transition to an arbitrary full camera pose. Decomposes both the
    // current pose and the destination pose into (target, direction, distance, up) and
    // interpolates all four independently in update(); when the destination target/distance
    // equal the current ones (as go_to_preset_view always arranges), this reduces exactly to
    // rotating around a fixed pivot at a fixed radius.
    void begin_pose_transition(const linal::double3& endPosition,
                              const linal::double3& endTarget,
                              const linal::double3& endUp) {
        const linal::double3 startPos = to_linal(m_camera.get_position());
        const linal::double3 startTarget = to_linal(m_camera.get_target());
        const double startDistance = linal::length(startPos - startTarget);
        const double endDistance = linal::length(endPosition - endTarget);
        RENDERER_ASSERT(startDistance > 1.0e-9);
        RENDERER_ASSERT(endDistance > 1.0e-9);

        m_transitionStartTarget = startTarget;
        m_transitionEndTarget = endTarget;
        m_transitionStartDirection = linal::normalize(startPos - startTarget);
        m_transitionEndDirection = linal::normalize(endPosition - endTarget);
        m_transitionStartDistance = startDistance;
        m_transitionEndDistance = endDistance;
        m_transitionStartUp = to_linal(m_camera.get_vertical());
        m_transitionEndUp = endUp;
        m_transitionElapsedSeconds = 0.0;
        m_isTransitioning = m_transitionDurationSeconds > 0.0;

        if (!m_isTransitioning) {
            m_camera.look_at(to_glm(endPosition), to_glm(endTarget), to_glm(endUp));
            update_mvp();
        }
    }

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
