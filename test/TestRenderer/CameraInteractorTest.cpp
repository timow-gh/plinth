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

    interactor.on_mouse_button(GLFW_MOUSE_BUTTON_MIDDLE, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(460.0, 300.0);
    interactor.on_mouse_button(GLFW_MOUSE_BUTTON_MIDDLE, renderer::Action::RELEASE, renderer::Mods::NONE);

    EXPECT_TRUE(interactor.get_was_blocking());
    EXPECT_NE(interactor.get_position(), initialPosition);
    EXPECT_NE(interactor.get_target(), initialTarget);
}

TEST(CameraInteractorTest, PanAsFirstCursorEventDoesNotMoveOrCorruptCamera) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    const linal::double3 initialPosition = interactor.get_position();

    interactor.on_mouse_button(GLFW_MOUSE_BUTTON_MIDDLE, renderer::Action::PRESS, renderer::Mods::NONE);
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

    interactor.on_mouse_button(GLFW_MOUSE_BUTTON_MIDDLE, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(460.0, 300.0);
    interactor.on_mouse_button(GLFW_MOUSE_BUTTON_MIDDLE, renderer::Action::RELEASE, renderer::Mods::NONE);

    EXPECT_TRUE(interactor.get_was_blocking());
    EXPECT_NE(interactor.get_position(), initialPosition);
}

TEST(CameraInteractorTest, TinyCursorMoveDoesNotBlock) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    interactor.on_cursor_position(400.0, 300.0);
    interactor.on_mouse_button(GLFW_MOUSE_BUTTON_MIDDLE, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(400.0001, 300.0001);

    EXPECT_FALSE(interactor.get_was_blocking());
}

TEST(CameraInteractorTest, RightMouseDragOrbitsCamera) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    interactor.on_cursor_position(400.0, 300.0);
    const linal::double3 initialPosition = interactor.get_position();

    interactor.on_mouse_button(GLFW_MOUSE_BUTTON_RIGHT, renderer::Action::PRESS, renderer::Mods::NONE);
    interactor.on_cursor_position(410.0, 300.0);
    interactor.on_cursor_position(440.0, 300.0);
    interactor.on_mouse_button(GLFW_MOUSE_BUTTON_RIGHT, renderer::Action::RELEASE, renderer::Mods::NONE);

    EXPECT_TRUE(interactor.get_was_blocking());
    EXPECT_NE(interactor.get_position(), initialPosition);
}

TEST(CameraInteractorTest, UnsupportedMouseButtonDoesNotBlock) {
    renderer::InputState inputState;
    renderer::CameraInteractor interactor = make_interactor(inputState);

    interactor.on_mouse_button(GLFW_MOUSE_BUTTON_LEFT, renderer::Action::PRESS, renderer::Mods::NONE);

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
