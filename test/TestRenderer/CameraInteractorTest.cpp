#include <plinth/CameraInteractor.hpp>
#include <gtest/gtest.h>
#include <cmath>

namespace {

constexpr double tolerance = 1.0e-6;

renderer::CameraInteractor make_interactor(renderer::InputState& inputState) {
    renderer::CameraSettings settings;
    settings.m_defaultPosition = linal::double3{0.0, -10.0, 10.0};
    settings.m_defaultTarget = linal::double3{0.0, 0.0, 0.0};
    settings.m_defaultUp = linal::double3{0.0, 0.0, 1.0};
    renderer::CameraInteractor interactor(inputState, settings);
    interactor.set_viewport(0, 0, 800, 600);
    return interactor;
}

} // namespace

TEST(CameraInteractorTest, AppliesProjectionAndClippingSettings) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    EXPECT_EQ(interactor.get_default_position(), (linal::double3{0.0, -10.0, 10.0}));
    EXPECT_EQ(interactor.get_default_target(), (linal::double3{0.0, 0.0, 0.0}));
    EXPECT_EQ(interactor.get_default_up(), (linal::double3{0.0, 0.0, 1.0}));

    interactor.set_near_plane(0.5);
    interactor.set_far_plane(500.0);
    interactor.set_zoom_factor(0.3);
    EXPECT_DOUBLE_EQ(interactor.get_near_plane(), 0.5);
    EXPECT_DOUBLE_EQ(interactor.get_far_plane(), 500.0);
    (void)interactor.get_projection_matrix();

    interactor.set_projection_type(renderer::CameraProjectionType::ORTHOGRAPHIC);
    interactor.set_zoom_factor(0.1);
    interactor.set_near_plane(0.25);
    interactor.set_far_plane(250.0);
    EXPECT_EQ(interactor.get_projection_type(), renderer::CameraProjectionType::ORTHOGRAPHIC);
    EXPECT_DOUBLE_EQ(interactor.get_near_plane(), 0.25);
    EXPECT_DOUBLE_EQ(interactor.get_far_plane(), 250.0);

    const renderer::PickRay ray = interactor.get_pick_ray(400.0, 300.0);
    EXPECT_NEAR(linal::length(ray.direction), 1.0, tolerance);
    (void)interactor.get_projection_matrix();
    (void)interactor.get_current_MVP();
    (void)interactor.get_normal_matrix();
}

TEST(CameraInteractorTest, MoveTransfersInteractiveState) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);
    inputState.cursorPosState = renderer::CursorPosState{400.0, 300.0};
    interactor.on_scroll(0.0, 1.0);
    const linal::double3 scrolledPosition = interactor.get_position();

    renderer::CameraInteractor moved(std::move(interactor));
    EXPECT_EQ(moved.get_position(), scrolledPosition);

    renderer::InputState otherInputState;
    renderer::CameraInteractor assigned = make_interactor(otherInputState);
    assigned = std::move(moved);
    EXPECT_EQ(assigned.get_position(), scrolledPosition);
}

TEST(CameraInteractorTest, AppliesAutoFitResultToOrthographicCamera) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);
    interactor.set_projection_type(renderer::CameraProjectionType::ORTHOGRAPHIC);

    renderer::CameraAutoFitResult result;
    result.position = linal::double3{1.0, 2.0, 30.0};
    result.target = linal::double3{1.0, 2.0, 0.0};
    result.vertical = linal::double3{0.0, 1.0, 0.0};
    result.orthographicWidth = 25.0;
    result.orthographicHeight = 15.0;
    result.farPlane = 750.0;

    interactor.apply_auto_fit_result(result);

    EXPECT_EQ(interactor.get_position(), result.position);
    EXPECT_EQ(interactor.get_target(), result.target);
    EXPECT_EQ(interactor.get_vertical(), result.vertical);
    EXPECT_DOUBLE_EQ(interactor.get_orthographic_params().width, 25.0);
    EXPECT_DOUBLE_EQ(interactor.get_orthographic_params().height, 15.0);
    EXPECT_DOUBLE_EQ(interactor.get_far_plane(), 750.0);
}

TEST(CameraInteractorTest, ScrollZoomsPerspectiveCameraAndSetsBlocking) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);
    inputState.cursorPosState = renderer::CursorPosState{400.0, 300.0};

    const linal::double3 initialPosition = interactor.get_position();
    interactor.on_scroll(0.0, 1.0);

    EXPECT_TRUE(interactor.get_was_blocking());
    EXPECT_LT(linal::length(interactor.get_position() - interactor.get_target()),
              linal::length(initialPosition - interactor.get_target()));

    interactor.reset_was_blocking();
    EXPECT_FALSE(interactor.get_was_blocking());
}

TEST(CameraInteractorTest, ScrollZoomsOrthographicCamera) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);
    interactor.set_projection_type(renderer::CameraProjectionType::ORTHOGRAPHIC);
    inputState.cursorPosState = renderer::CursorPosState{400.0, 300.0};

    const double initialWidth = interactor.get_orthographic_params().width;
    interactor.on_scroll(0.0, 1.0);

    EXPECT_TRUE(interactor.get_was_blocking());
    EXPECT_LT(interactor.get_orthographic_params().width, initialWidth);
}

TEST(CameraInteractorTest, ScrollDoesNotBlockWhenGroundPlaneIsNotHit) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);
    inputState.cursorPosState = renderer::CursorPosState{400.0, 300.0};
    interactor.set_ground_plane(renderer::Plane{linal::double3{0.0, 0.0, 100.0}, linal::double3{1.0, 0.0, 0.0}});

    const linal::double3 initialPosition = interactor.get_position();
    interactor.on_scroll(0.0, 1.0);

    EXPECT_FALSE(interactor.get_was_blocking());
    EXPECT_EQ(interactor.get_position(), initialPosition);
}

TEST(CameraInteractorTest, MiddleMouseDragPansCamera) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    interactor.on_cursor_position(400.0, 300.0);
    const linal::double3 initialPosition = interactor.get_position();
    const linal::double3 initialTarget = interactor.get_target();

    interactor.on_mouse_button(2, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(460.0, 300.0);
    interactor.on_mouse_button(2, renderer::Action::RELEASE, renderer::Mods::NONE);

    EXPECT_TRUE(interactor.get_was_blocking());
    EXPECT_NE(interactor.get_position(), initialPosition);
    EXPECT_NE(interactor.get_target(), initialTarget);
}

TEST(CameraInteractorTest, PanAsFirstCursorEventDoesNotMoveOrCorruptCamera) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    const linal::double3 initialPosition = interactor.get_position();

    interactor.on_mouse_button(2, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(460.0, 300.0);

    EXPECT_EQ(interactor.get_position(), initialPosition);
    EXPECT_TRUE(std::isfinite(interactor.get_position()[0]));
    EXPECT_TRUE(std::isfinite(interactor.get_position()[1]));
    EXPECT_TRUE(std::isfinite(interactor.get_position()[2]));

    interactor.on_cursor_position(520.0, 300.0);

    EXPECT_NE(interactor.get_position(), initialPosition);
}

TEST(CameraInteractorTest, MiddleMouseDragPansOrthographicCamera) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);
    interactor.set_projection_type(renderer::CameraProjectionType::ORTHOGRAPHIC);

    interactor.on_cursor_position(400.0, 300.0);
    const linal::double3 initialPosition = interactor.get_position();

    interactor.on_mouse_button(2, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(460.0, 300.0);
    interactor.on_mouse_button(2, renderer::Action::RELEASE, renderer::Mods::NONE);

    EXPECT_TRUE(interactor.get_was_blocking());
    EXPECT_NE(interactor.get_position(), initialPosition);
}

TEST(CameraInteractorTest, TinyCursorMoveDoesNotBlock) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    interactor.on_cursor_position(400.0, 300.0);
    interactor.on_mouse_button(2, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(400.0001, 300.0001);

    EXPECT_FALSE(interactor.get_was_blocking());
}

TEST(CameraInteractorTest, RightMouseDragOrbitsCamera) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    interactor.on_cursor_position(400.0, 300.0);
    const linal::double3 initialPosition = interactor.get_position();

    interactor.on_mouse_button(1, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(410.0, 300.0);
    interactor.on_cursor_position(440.0, 300.0);
    interactor.on_mouse_button(1, renderer::Action::RELEASE, renderer::Mods::NONE);

    EXPECT_TRUE(interactor.get_was_blocking());
    EXPECT_NE(interactor.get_position(), initialPosition);
}

TEST(CameraInteractorTest, UnsupportedMouseButtonDoesNotBlock) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    interactor.on_mouse_button(0, renderer::Action::PRESS, renderer::Mods::NONE);

    EXPECT_FALSE(interactor.get_was_blocking());
}

TEST(CameraInteractorTest, FramebufferResizeUpdatesViewport) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    interactor.on_framebuffer_size(1024, 512);

    EXPECT_EQ(interactor.get_viewport().get_width(), 1024);
    EXPECT_EQ(interactor.get_viewport().get_height(), 512);
}

TEST(CameraInteractorTest, ZeroFramebufferResizeKeepsCurrentViewport) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    interactor.on_framebuffer_size(1024, 512);
    interactor.on_framebuffer_size(0, 512);
    interactor.on_framebuffer_size(1024, 0);

    EXPECT_EQ(interactor.get_viewport().get_width(), 1024);
    EXPECT_EQ(interactor.get_viewport().get_height(), 512);
}

TEST(CameraInteractorTest, FlyModeIgnoresKeysInOrbitStyle) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    const linal::double3 initialPosition = interactor.get_position();
    const linal::double3 initialTarget = interactor.get_target();

    interactor.update(1.0, [](renderer::Key key) { return key == renderer::Key::KEY_W; });

    EXPECT_EQ(interactor.get_position(), initialPosition);
    EXPECT_EQ(interactor.get_target(), initialTarget);
}

TEST(CameraInteractorTest, FlyModeMovesForwardOnW) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);
    interactor.set_navigation_style(renderer::CameraInteractor::NavigationStyle::FLY);

    const linal::double3 initialPosition = interactor.get_position();
    const linal::double3 initialTarget = interactor.get_target();
    const linal::double3 forward = linal::normalize(initialTarget - initialPosition);

    interactor.update(1.0, [](renderer::Key key) { return key == renderer::Key::KEY_W; });

    const linal::double3 translation = interactor.get_position() - initialPosition;
    EXPECT_NEAR(linal::length(translation), interactor.get_fly_speed(), 1.0e-6);
    EXPECT_NEAR(linal::dot(linal::normalize(translation), forward), 1.0, 1.0e-6);
    EXPECT_TRUE(interactor.get_was_blocking());
}

TEST(CameraInteractorTest, FlyModeStrafeAndVerticalKeysCombine) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);
    interactor.set_navigation_style(renderer::CameraInteractor::NavigationStyle::FLY);

    const linal::double3 initialPosition = interactor.get_position();
    const linal::double3 initialTarget = interactor.get_target();
    const linal::double3 forward = linal::normalize(initialTarget - initialPosition);
    const linal::double3 up = interactor.get_vertical();
    const linal::double3 right = linal::normalize(linal::cross(forward, up));

    interactor.update(1.0, [](renderer::Key key) {
        return key == renderer::Key::KEY_D || key == renderer::Key::KEY_E;
    });

    const linal::double3 translation = interactor.get_position() - initialPosition;
    EXPECT_GT(linal::dot(translation, right), 0.0);
    EXPECT_GT(linal::dot(translation, up), 0.0);
}

TEST(CameraInteractorTest, FlyModeZeroDeltaSecondsIsNoOp) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);
    interactor.set_navigation_style(renderer::CameraInteractor::NavigationStyle::FLY);

    const linal::double3 initialPosition = interactor.get_position();
    const linal::double3 initialTarget = interactor.get_target();

    interactor.update(0.0, [](renderer::Key) { return true; });

    EXPECT_EQ(interactor.get_position(), initialPosition);
    EXPECT_EQ(interactor.get_target(), initialTarget);
    EXPECT_FALSE(interactor.get_was_blocking());
}

TEST(CameraInteractorTest, FlyModeMouseLookRotatesWithoutGroundPlane) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);
    // Mirrors ScrollDoesNotBlockWhenGroundPlaneIsNotHit's plane setup: a plane the gaze ray will never hit.
    interactor.set_ground_plane(renderer::Plane{linal::double3{0.0, 0.0, 100.0}, linal::double3{1.0, 0.0, 0.0}});
    interactor.set_navigation_style(renderer::CameraInteractor::NavigationStyle::FLY);

    interactor.on_cursor_position(400.0, 300.0);
    const linal::double3 initialPosition = interactor.get_position();
    const linal::double3 initialGaze = linal::normalize(interactor.get_target() - initialPosition);

    interactor.on_mouse_button(1, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(410.0, 300.0);
    interactor.on_cursor_position(440.0, 300.0);
    interactor.on_mouse_button(1, renderer::Action::RELEASE, renderer::Mods::NONE);

    const linal::double3 newGaze = linal::normalize(interactor.get_target() - interactor.get_position());

    EXPECT_TRUE(interactor.get_was_blocking());
    // Fly mouse-look rotates the gaze direction in place, unlike orbit which rotates the eye around a
    // ground-plane pivot; position should be unaffected but the gaze direction should change.
    EXPECT_EQ(interactor.get_position(), initialPosition);
    EXPECT_GT(linal::length(newGaze - initialGaze), 1.0e-6);
}

namespace {
bool no_key_pressed(renderer::Key) {
    return false;
}
} // namespace

TEST(CameraInteractorTest, PresetViewSnapsInstantlyWhenDurationIsZero) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    const linal::double3 target = interactor.get_target();
    const double distance = linal::length(interactor.get_position() - target);

    interactor.set_view_transition_duration(0.0);
    interactor.go_to_preset_view(renderer::CameraInteractor::PresetView::TOP);

    EXPECT_FALSE(interactor.is_transitioning_view());
    const linal::double3 expectedPosition = target + linal::double3{0.0, 0.0, 1.0} * distance;
    EXPECT_NEAR(linal::length(interactor.get_position() - expectedPosition), 0.0, tolerance);
}

TEST(CameraInteractorTest, PresetViewTransitionPreservesDistanceThroughoutInterpolation) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    const linal::double3 target = interactor.get_target();
    const double distance = linal::length(interactor.get_position() - target);

    constexpr double duration = 0.4;
    interactor.set_view_transition_duration(duration);
    interactor.go_to_preset_view(renderer::CameraInteractor::PresetView::RIGHT);

    constexpr int steps = 10;
    constexpr double stepSeconds = duration / steps;
    for (int i = 0; i < steps; ++i) {
        interactor.update(stepSeconds, no_key_pressed);
        const double currentDistance = linal::length(interactor.get_position() - interactor.get_target());
        EXPECT_NEAR(currentDistance, distance, 1.0e-6);
    }
}

TEST(CameraInteractorTest, PresetViewTransitionReachesExactTargetAtOrAfterDuration) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    const linal::double3 target = interactor.get_target();
    const double distance = linal::length(interactor.get_position() - target);

    constexpr double duration = 0.4;
    interactor.set_view_transition_duration(duration);
    interactor.go_to_preset_view(renderer::CameraInteractor::PresetView::ISO);

    interactor.update(duration * 2.0, no_key_pressed);

    EXPECT_FALSE(interactor.is_transitioning_view());
    const linal::double3 isoDirection = linal::normalize(linal::double3{1.0, -1.0, 1.0});
    const linal::double3 expectedPosition = target + isoDirection * distance;
    EXPECT_NEAR(linal::length(interactor.get_position() - expectedPosition), 0.0, 1.0e-6);
}

TEST(CameraInteractorTest, PresetViewTransitionIsMonotonicInProgress) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    constexpr double duration = 0.4;
    interactor.set_view_transition_duration(duration);
    interactor.go_to_preset_view(renderer::CameraInteractor::PresetView::BACK);

    const linal::double3 endDirection = linal::normalize(linal::double3{0.0, 1.0, 0.0});

    constexpr int steps = 10;
    constexpr double stepSeconds = duration / steps;
    double previousAngle = std::numeric_limits<double>::max();
    for (int i = 0; i < steps; ++i) {
        interactor.update(stepSeconds, no_key_pressed);
        const linal::double3 currentDirection =
            linal::normalize(interactor.get_position() - interactor.get_target());
        const double cosAngle = std::clamp(linal::dot(currentDirection, endDirection), -1.0, 1.0);
        const double angle = std::acos(cosAngle);
        EXPECT_LE(angle, previousAngle + 1.0e-9);
        previousAngle = angle;
    }
}

TEST(CameraInteractorTest, PresetViewTopAndBottomUseAlternateUpVector) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    interactor.set_view_transition_duration(0.0);
    interactor.go_to_preset_view(renderer::CameraInteractor::PresetView::TOP);

    EXPECT_NEAR(linal::length(interactor.get_vertical() - linal::double3{0.0, 1.0, 0.0}), 0.0, tolerance);
}

TEST(CameraInteractorTest, OrbitSurvivesUnrelatedButtonRelease) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    interactor.on_cursor_position(400.0, 300.0);
    interactor.on_mouse_button(1, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(440.0, 300.0);
    const linal::double3 afterFirstDrag = interactor.get_position();

    // Click the middle button mid-orbit: press is ignored, release must not
    // cancel the right-button orbit gesture.
    interactor.on_mouse_button(2, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_mouse_button(2, renderer::Action::RELEASE, renderer::Mods::NONE);
    interactor.on_cursor_position(480.0, 300.0);

    EXPECT_NE(interactor.get_position(), afterFirstDrag);

    // Releasing the right button really ends the gesture.
    interactor.on_mouse_button(1, renderer::Action::RELEASE, renderer::Mods::NONE);
    const linal::double3 afterOrbitRelease = interactor.get_position();
    interactor.on_cursor_position(520.0, 300.0);
    EXPECT_EQ(interactor.get_position(), afterOrbitRelease);
}

TEST(CameraInteractorTest, PanSurvivesUnrelatedButtonRelease) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    interactor.on_cursor_position(400.0, 300.0);
    interactor.on_mouse_button(2, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(440.0, 300.0);
    const linal::double3 afterFirstDrag = interactor.get_position();

    interactor.on_mouse_button(1, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_mouse_button(1, renderer::Action::RELEASE, renderer::Mods::NONE);
    interactor.on_cursor_position(480.0, 300.0);

    EXPECT_NE(interactor.get_position(), afterFirstDrag);

    interactor.on_mouse_button(2, renderer::Action::RELEASE, renderer::Mods::NONE);
    const linal::double3 afterPanRelease = interactor.get_position();
    interactor.on_cursor_position(520.0, 300.0);
    EXPECT_EQ(interactor.get_position(), afterPanRelease);
}

TEST(CameraInteractorTest, PresetViewTransitionBetweenExactOppositesStaysFinite) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    const linal::double3 target = interactor.get_target();
    const double distance = linal::length(interactor.get_position() - target);

    constexpr double duration = 0.4;
    interactor.set_view_transition_duration(duration);
    interactor.go_to_preset_view(renderer::CameraInteractor::PresetView::FRONT);
    interactor.update(duration * 2.0, no_key_pressed);
    interactor.go_to_preset_view(renderer::CameraInteractor::PresetView::BACK);

    constexpr int steps = 10;
    constexpr double stepSeconds = duration / steps;
    for (int i = 0; i < steps; ++i) {
        interactor.update(stepSeconds, no_key_pressed);
        const linal::double3 position = interactor.get_position();
        EXPECT_TRUE(std::isfinite(position[0]));
        EXPECT_TRUE(std::isfinite(position[1]));
        EXPECT_TRUE(std::isfinite(position[2]));
        EXPECT_NEAR(linal::length(interactor.get_position() - interactor.get_target()), distance, 1.0e-6);
    }

    const linal::double3 backDirection = linal::normalize(linal::double3{0.0, 1.0, 0.0});
    const linal::double3 expectedPosition = target + backDirection * distance;
    EXPECT_NEAR(linal::length(interactor.get_position() - expectedPosition), 0.0, 1.0e-6);
}

TEST(CameraInteractorTest, PoseTransitionTargetMovesMonotonicallyTowardEndTarget) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    const linal::double3 endPosition{5.0, 5.0, 5.0};
    const linal::double3 endTarget{2.0, 3.0, 1.0};
    const linal::double3 endUp{0.0, 0.0, 1.0};

    constexpr double duration = 0.4;
    interactor.set_view_transition_duration(duration);
    interactor.transition_to_pose(endPosition, endTarget, endUp);

    constexpr int steps = 10;
    constexpr double stepSeconds = duration / steps;
    double previousDistanceToEnd = linal::length(interactor.get_target() - endTarget);
    for (int i = 0; i < steps; ++i) {
        interactor.update(stepSeconds, no_key_pressed);
        const double distanceToEnd = linal::length(interactor.get_target() - endTarget);
        EXPECT_LE(distanceToEnd, previousDistanceToEnd + 1.0e-9);
        previousDistanceToEnd = distanceToEnd;
    }
}

TEST(CameraInteractorTest, PoseTransitionInterpolatesDistanceWhenEndDistanceDiffers) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    const linal::double3 target = interactor.get_target();
    const double startDistance = linal::length(interactor.get_position() - target);
    const linal::double3 direction = linal::normalize(interactor.get_position() - target);
    const double endDistance = startDistance * 3.0;
    const linal::double3 endPosition = target + direction * endDistance;

    constexpr double duration = 0.4;
    interactor.set_view_transition_duration(duration);
    interactor.transition_to_pose(endPosition, target, interactor.get_vertical());

    constexpr int steps = 10;
    constexpr double stepSeconds = duration / steps;
    double previousDistance = startDistance;
    for (int i = 0; i < steps; ++i) {
        interactor.update(stepSeconds, no_key_pressed);
        const double currentDistance = linal::length(interactor.get_position() - interactor.get_target());
        EXPECT_GE(currentDistance, previousDistance - 1.0e-9);
        previousDistance = currentDistance;
    }
    EXPECT_GT(previousDistance, startDistance + tolerance);
}

TEST(CameraInteractorTest, PoseTransitionSnapsInstantlyWhenDurationIsZero) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    const linal::double3 endPosition{5.0, 5.0, 5.0};
    const linal::double3 endTarget{2.0, 3.0, 1.0};
    const linal::double3 endUp{0.0, 0.0, 1.0};

    interactor.set_view_transition_duration(0.0);
    interactor.transition_to_pose(endPosition, endTarget, endUp);

    EXPECT_FALSE(interactor.is_transitioning_view());
    EXPECT_NEAR(linal::length(interactor.get_position() - endPosition), 0.0, tolerance);
    EXPECT_NEAR(linal::length(interactor.get_target() - endTarget), 0.0, tolerance);
    EXPECT_NEAR(linal::length(interactor.get_vertical() - endUp), 0.0, tolerance);
}

TEST(CameraInteractorTest, PoseTransitionReachesExactEndPoseAtOrAfterDuration) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    const linal::double3 endPosition{5.0, 5.0, 5.0};
    const linal::double3 endTarget{2.0, 3.0, 1.0};
    const linal::double3 endUp{0.0, 0.0, 1.0};

    constexpr double duration = 0.4;
    interactor.set_view_transition_duration(duration);
    interactor.transition_to_pose(endPosition, endTarget, endUp);

    interactor.update(duration * 2.0, no_key_pressed);

    EXPECT_FALSE(interactor.is_transitioning_view());
    EXPECT_NEAR(linal::length(interactor.get_position() - endPosition), 0.0, 1.0e-6);
    EXPECT_NEAR(linal::length(interactor.get_target() - endTarget), 0.0, 1.0e-6);
}

TEST(CameraInteractorTest, RightDragPivotsAroundCursorGroundIntersection) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    // Cursor off-center: the pivot is where the ray through this cursor position hits the ground
    // plane (z=0), which is different from the gaze-ray (screen center) intersection at the origin.
    constexpr double cursorX = 250.0;
    constexpr double cursorY = 200.0;
    inputState.cursorPosState = renderer::CursorPosState{cursorX, cursorY};

    linal::double3 expectedPivot;
    const renderer::PickRay ray = interactor.get_pick_ray(cursorX, cursorY);
    ASSERT_TRUE(renderer::ray_plane_intersection(interactor.get_position(),
                                                 ray.direction,
                                                 renderer::Plane{linal::double3{0.0, 0.0, 0.0},
                                                                 linal::double3{0.0, 0.0, 1.0}},
                                                 expectedPivot));
    ASSERT_GT(linal::length(expectedPivot), tolerance); // off-center: not the world origin

    interactor.on_cursor_position(cursorX, cursorY);
    const double distanceToPivot = linal::length(interactor.get_position() - expectedPivot);

    interactor.on_mouse_button(1, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(cursorX + 20.0, cursorY);
    interactor.on_cursor_position(cursorX + 50.0, cursorY);
    interactor.on_mouse_button(1, renderer::Action::RELEASE, renderer::Mods::NONE);

    // Orbiting keeps the camera a constant distance from the (cursor-based) pivot.
    EXPECT_TRUE(interactor.get_was_blocking());
    EXPECT_NEAR(linal::length(interactor.get_position() - expectedPivot), distanceToPivot, 1.0e-6);
}

TEST(CameraInteractorTest, RightDragDoesNotSnapGazeOntoCursorPivot) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    // Cursor off-center so the pivot differs from both the camera target and the screen-center gaze
    // point. The camera must rotate the scene around that pivot without the gaze snapping to look at
    // it (which previously caused a visible jump on the first rotate event).
    constexpr double cursorX = 250.0;
    constexpr double cursorY = 200.0;
    inputState.cursorPosState = renderer::CursorPosState{cursorX, cursorY};

    interactor.on_cursor_position(cursorX, cursorY);
    const linal::double3 initialTarget = interactor.get_target();
    const double initialEyeTargetDist = linal::length(interactor.get_position() - initialTarget);

    linal::double3 expectedPivot;
    const renderer::PickRay ray = interactor.get_pick_ray(cursorX, cursorY);
    ASSERT_TRUE(renderer::ray_plane_intersection(interactor.get_position(),
                                                 ray.direction,
                                                 renderer::Plane{linal::double3{0.0, 0.0, 0.0},
                                                                 linal::double3{0.0, 0.0, 1.0}},
                                                 expectedPivot));

    interactor.on_mouse_button(1, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(cursorX + 20.0, cursorY);
    interactor.on_mouse_button(1, renderer::Action::RELEASE, renderer::Mods::NONE);

    // The gaze target must not have collapsed onto the pivot (the old jump), and rigid rotation
    // preserves the eye-to-target distance so the framing does not change.
    EXPECT_GT(linal::length(interactor.get_target() - expectedPivot), tolerance);
    EXPECT_NEAR(linal::length(interactor.get_position() - interactor.get_target()),
                initialEyeTargetDist,
                1.0e-6);
}

TEST(CameraInteractorTest, RightDragDoesNotOrbitWhenCursorRayMissesGroundPlane) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);
    inputState.cursorPosState = renderer::CursorPosState{400.0, 300.0};
    // A ground plane the cursor ray will never hit (mirrors ScrollDoesNotBlockWhenGroundPlaneIsNotHit).
    interactor.set_ground_plane(renderer::Plane{linal::double3{0.0, 0.0, 100.0}, linal::double3{1.0, 0.0, 0.0}});

    interactor.on_cursor_position(400.0, 300.0);
    const linal::double3 initialPosition = interactor.get_position();

    interactor.on_mouse_button(1, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(430.0, 300.0);
    interactor.on_cursor_position(460.0, 300.0);
    interactor.on_mouse_button(1, renderer::Action::RELEASE, renderer::Mods::NONE);

    EXPECT_FALSE(interactor.get_was_blocking());
    EXPECT_EQ(interactor.get_position(), initialPosition);
}

TEST(CameraInteractorTest, OriginPivotModeOrbitsAroundWorldOrigin) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);
    interactor.set_pivot_mode(renderer::CameraInteractor::PivotMode::ORIGIN);

    const double distanceToOrigin = linal::length(interactor.get_position());

    interactor.on_cursor_position(400.0, 300.0);
    interactor.on_mouse_button(1, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(430.0, 320.0);
    interactor.on_cursor_position(470.0, 340.0);
    interactor.on_mouse_button(1, renderer::Action::RELEASE, renderer::Mods::NONE);

    EXPECT_TRUE(interactor.get_was_blocking());
    // The eye orbits the world origin at constant radius, and the gaze target is the origin.
    EXPECT_NEAR(linal::length(interactor.get_position()), distanceToOrigin, 1.0e-6);
    EXPECT_NEAR(linal::length(interactor.get_target()), 0.0, 1.0e-6);
}

TEST(CameraInteractorTest, OriginPivotModeAllowsPanning) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);
    interactor.set_pivot_mode(renderer::CameraInteractor::PivotMode::ORIGIN);

    interactor.on_cursor_position(400.0, 300.0);
    const linal::double3 initialPosition = interactor.get_position();

    interactor.on_mouse_button(2, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(460.0, 300.0);
    interactor.on_mouse_button(2, renderer::Action::RELEASE, renderer::Mods::NONE);

    EXPECT_TRUE(interactor.get_was_blocking());
    EXPECT_NE(interactor.get_position(), initialPosition);
}

TEST(CameraInteractorTest, OriginPivotModeStillAllowsScrollZoom) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);
    interactor.set_pivot_mode(renderer::CameraInteractor::PivotMode::ORIGIN);
    inputState.cursorPosState = renderer::CursorPosState{400.0, 300.0};

    const linal::double3 initialPosition = interactor.get_position();
    interactor.on_scroll(0.0, 1.0);

    EXPECT_TRUE(interactor.get_was_blocking());
    EXPECT_NE(interactor.get_position(), initialPosition);
}
