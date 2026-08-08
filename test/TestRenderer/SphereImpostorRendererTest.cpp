#include "plinth/CameraInteractor.hpp"
#include "plinth/Renderer.hpp"
#include "plinth/WindowSettings.hpp"

#include <glad/glad.h>

#include <linal/vec.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <numbers>
#include <numeric>
#include <span>
#include <vector>

namespace {

const std::vector<float> kCenters{0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F};
const std::vector<float> kRadii{0.5F, 0.5F};
const std::vector<float> kColors{1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F};
const std::vector<float> kTranslucentColors{1.0F, 0.0F, 0.0F, 0.5F, 0.0F, 1.0F, 0.0F, 0.5F};
constexpr std::array<float, 4> kUniformColor{0.5F, 0.5F, 0.5F, 1.0F};

class SphereImpostorRendererTest : public ::testing::Test {
  protected:
    static std::unique_ptr<renderer::Renderer> create_renderer() {
        renderer::WindowSettings settings;
        settings.title = "sphere impostor renderer test";
        settings.width = 256U;
        settings.height = 256U;
        settings.visible = false;
        settings.resizable = false;
        settings.double_buffer = false;
        settings.srgb_capable = false;
        settings.samples = 1;
        return renderer::Renderer::create(settings);
    }

    static void render_one_frame(renderer::Renderer& renderer) {
        renderer.begin_frame({0.0F, 0.0F, 0.0F, 1.0F});
        renderer.draw();
        renderer.end_frame();
        glFinish();
    }

    // A renderer configured for deterministic front-buffer readback of the presented scene:
    // single-buffered, no MSAA/sRGB, no overlay panel covering the scene, and FXAA disabled so
    // the sampled interior pixel is the sphere color rather than an edge-blended value.
    static std::unique_ptr<renderer::Renderer> create_readback_renderer(std::uint32_t width = 256U,
                                                                        std::uint32_t height = 256U) {
        renderer::WindowSettings settings;
        settings.title = "sphere impostor readback test";
        settings.width = width;
        settings.height = height;
        settings.visible = false;
        settings.resizable = false;
        settings.double_buffer = false;
        settings.srgb_capable = false;
        settings.samples = 1;
        settings.overlay = renderer::OverlayKind::None;
        auto instance = renderer::Renderer::create(settings);
        if (instance) {
            instance->set_fxaa_enabled(false);
        }
        return instance;
    }

    static std::array<std::uint8_t, 4> read_front_pixel(int x, int y) {
        std::array<std::uint8_t, 4> pixel{};
        glReadBuffer(GL_FRONT);
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
        return pixel;
    }

    static std::pair<int, int> scene_interior_coordinate(renderer::Renderer& renderer) {
        const auto sceneViewport = renderer.scene_viewport();
        return {sceneViewport.framebuffer.x + sceneViewport.framebuffer.width / 2,
                sceneViewport.framebuffer.y + sceneViewport.framebuffer.height / 2};
    }
};

// ──────────────────────────────────────────────────────────────────────────────
// add_sphere_point_drawable
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(SphereImpostorRendererTest, AddPerVertexColorsReturnsValidSphereHandle) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    const renderer::DrawableHandle handle = instance->add_sphere_point_drawable(kCenters, kRadii, kColors);
    ASSERT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::sphere, handle.kind);
}

TEST_F(SphereImpostorRendererTest, AddUniformColorReturnsValidSphereHandle) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    const renderer::DrawableHandle handle = instance->add_sphere_point_drawable(kCenters, kRadii, kUniformColor);
    ASSERT_TRUE(handle.is_valid());
    EXPECT_EQ(renderer::DrawableKind::sphere, handle.kind);
}

TEST_F(SphereImpostorRendererTest, AddMismatchedSizesReturnsInvalidHandle) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    // 3 centers but 2 radii — size mismatch
    const std::vector<float> threeCenters{0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 2.0F, 0.0F, 0.0F};
    const renderer::DrawableHandle handle = instance->add_sphere_point_drawable(threeCenters, kRadii, kColors);
    EXPECT_FALSE(handle.is_valid());
}

// ──────────────────────────────────────────────────────────────────────────────
// has_sphere_point_drawables / clear_sphere_point_drawables
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(SphereImpostorRendererTest, HasSphereDrawablesReflectsAddState) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    EXPECT_FALSE(instance->has_sphere_point_drawables());
    instance->add_sphere_point_drawable(kCenters, kRadii, kColors);
    EXPECT_TRUE(instance->has_sphere_point_drawables());
}

TEST_F(SphereImpostorRendererTest, ClearSphereDrawablesRemovesAll) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    instance->add_sphere_point_drawable(kCenters, kRadii, kColors);
    instance->add_sphere_point_drawable(kCenters, kRadii, kUniformColor);
    EXPECT_TRUE(instance->has_sphere_point_drawables());

    instance->clear_sphere_point_drawables();
    EXPECT_FALSE(instance->has_sphere_point_drawables());
}

TEST_F(SphereImpostorRendererTest, ClearDrawablesIncludesSpheres) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    instance->add_sphere_point_drawable(kCenters, kRadii, kColors);
    instance->clear_drawables();
    EXPECT_FALSE(instance->has_sphere_point_drawables());
}

// ──────────────────────────────────────────────────────────────────────────────
// remove_drawable
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(SphereImpostorRendererTest, RemoveDrawableSucceedsForSphereHandle) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    const renderer::DrawableHandle handle = instance->add_sphere_point_drawable(kCenters, kRadii, kColors);
    ASSERT_TRUE(handle.is_valid());
    EXPECT_TRUE(instance->remove_drawable(handle));
    EXPECT_FALSE(instance->has_sphere_point_drawables());
}

TEST_F(SphereImpostorRendererTest, RemoveDrawableReturnsFalseForStaleSphereHandle) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    const renderer::DrawableHandle handle = instance->add_sphere_point_drawable(kCenters, kRadii, kColors);
    ASSERT_TRUE(instance->remove_drawable(handle));
    EXPECT_FALSE(instance->remove_drawable(handle)); // stale
}

// ──────────────────────────────────────────────────────────────────────────────
// set_drawable_transform / get_drawable_transform
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(SphereImpostorRendererTest, SetAndGetTransformRoundTrips) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    const renderer::DrawableHandle handle = instance->add_sphere_point_drawable(kCenters, kRadii, kColors);
    ASSERT_TRUE(handle.is_valid());

    linal::hmatf t = linal::hmatf::identity();
    t.set_translation(linal::vec3<float>{1.0F, 2.0F, 3.0F});
    ASSERT_TRUE(instance->set_drawable_transform(handle, t));

    const auto retrieved = instance->get_drawable_transform(handle);
    ASSERT_TRUE(retrieved.has_value());
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            EXPECT_FLOAT_EQ(t(i, j), (*retrieved)(i, j));
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// set_sphere_point_size_space — handle validation
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(SphereImpostorRendererTest, SetSizeSpaceSucceedsForSphereHandle) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    const renderer::DrawableHandle handle = instance->add_sphere_point_drawable(kCenters, kRadii, kColors);
    ASSERT_TRUE(handle.is_valid());

    EXPECT_TRUE(instance->set_sphere_point_size_space(handle, renderer::SphereSizeSpace::Screen));
    EXPECT_TRUE(instance->set_sphere_point_size_space(handle, renderer::SphereSizeSpace::World));
}

TEST_F(SphereImpostorRendererTest, SetSizeSpaceRejectsInvalidAndForeignHandles) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    EXPECT_FALSE(instance->set_sphere_point_size_space(renderer::DrawableHandle{}, renderer::SphereSizeSpace::Screen));

    // A non-sphere handle of the same renderer must also be rejected.
    const renderer::DrawableHandle lineHandle =
        instance->add_line_drawable(kCenters, kColors, renderer::LineType::lines());
    ASSERT_TRUE(lineHandle.is_valid());
    EXPECT_FALSE(instance->set_sphere_point_size_space(lineHandle, renderer::SphereSizeSpace::Screen));
}

TEST_F(SphereImpostorRendererTest, ScreenSizeStyleAndSetterAcceptedAtAdd) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    const renderer::SphereStyle style{renderer::SphereSizeSpace::Screen};
    const renderer::DrawableHandle handle = instance->add_sphere_point_drawable(kCenters, kRadii, kColors, style);
    ASSERT_TRUE(handle.is_valid());
    render_one_frame(*instance);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

// ──────────────────────────────────────────────────────────────────────────────
// Screen-space size — the on-screen silhouette stays constant across camera distance,
// unlike world-space size which shrinks as the camera pulls back.
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(SphereImpostorRendererTest, ScreenSizeHoldsConstantSilhouetteAcrossDistance) {
    auto instance = create_readback_renderer();
    ASSERT_NE(nullptr, instance);

    const std::vector<float> center{0.0F, 0.0F, 0.0F};
    const std::array<float, 4> red{1.0F, 0.0F, 0.0F, 1.0F};

    renderer::LightingConfig flatLighting;
    flatLighting.lightColor = {0.0F, 0.0F, 0.0F};
    flatLighting.fillLightColor = {0.0F, 0.0F, 0.0F};
    flatLighting.ambientColor = {1.0F, 1.0F, 1.0F};
    flatLighting.materialAmbient = {1.0F, 1.0F, 1.0F};
    flatLighting.materialDiffuse = {0.0F, 0.0F, 0.0F};
    flatLighting.materialSpecular = {0.0F, 0.0F, 0.0F};

    // Count red pixels across the center row: the sphere's horizontal silhouette width. The camera
    // is placed explicitly before each draw, so the framing is deterministic and independent of the
    // renderer's auto-fit heuristics (auto-fit is disabled by default).
    const auto countRedRowPixels = [&](double distance) {
        auto camera = instance->get_camera().lock();
        camera->look_at({0.0, 0.0, distance}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});

        instance->begin_frame({0.0F, 0.0F, 0.0F, 1.0F});
        instance->draw(flatLighting);
        instance->end_frame();
        glFinish();

        const auto viewport = instance->scene_viewport();
        std::vector<std::uint8_t> row(static_cast<std::size_t>(viewport.framebuffer.width) * 4U);
        glReadBuffer(GL_FRONT);
        glReadPixels(viewport.framebuffer.x,
                     viewport.framebuffer.y + viewport.framebuffer.height / 2,
                     viewport.framebuffer.width,
                     1,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     row.data());
        std::size_t covered = 0;
        for (std::size_t pixel = 0; pixel < row.size() / 4U; ++pixel) {
            const std::uint8_t r = row[pixel * 4U];
            const std::uint8_t g = row[(pixel * 4U) + 1U];
            const std::uint8_t b = row[(pixel * 4U) + 2U];
            covered += r > 20U && g < 40U && b < 40U ? 1U : 0U;
        }
        return covered;
    };

    // A small world sphere and a moderate pixel radius that both stay comfortably inside the
    // 256x256 viewport at the near distance.
    const std::vector<float> worldRadius{0.5F};   // world units
    const std::vector<float> screenRadius{12.0F}; // pixels
    constexpr double kNearDistance = 6.0;
    constexpr double kFarDistance = 12.0;

    // World mode: radius is world units, so the silhouette shrinks when the camera pulls back.
    const renderer::DrawableHandle worldHandle = instance->add_sphere_point_drawable(
        center, worldRadius, red, renderer::SphereStyle{renderer::SphereSizeSpace::World});
    ASSERT_TRUE(worldHandle.is_valid());
    const std::size_t worldNear = countRedRowPixels(kNearDistance);
    const std::size_t worldFar = countRedRowPixels(kFarDistance);
    ASSERT_GT(worldNear, 0U);
    EXPECT_GT(worldNear, worldFar) << "world-space sphere should shrink with distance";
    instance->clear_sphere_point_drawables();

    // Screen mode: radius is pixels, so the silhouette is ~constant across distance.
    const renderer::DrawableHandle screenHandle = instance->add_sphere_point_drawable(
        center, screenRadius, red, renderer::SphereStyle{renderer::SphereSizeSpace::Screen});
    ASSERT_TRUE(screenHandle.is_valid());
    const std::size_t screenNear = countRedRowPixels(kNearDistance);
    const std::size_t screenFar = countRedRowPixels(kFarDistance);

    ASSERT_GT(screenNear, 0U);
    ASSERT_GT(screenFar, 0U);
    // Allow a few pixels of tolerance for rasterization/rounding; the point is that Screen mode
    // does not shrink the way World mode does.
    const std::size_t diff = screenNear > screenFar ? screenNear - screenFar : screenFar - screenNear;
    EXPECT_LE(diff, 3U) << "screen-space sphere should hold a constant pixel size across distance";
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

// ──────────────────────────────────────────────────────────────────────────────
// draw() — smoke tests (no crash, no GL error)
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(SphereImpostorRendererTest, DrawWithOpaqueSphereDoesNotCrash) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    instance->add_sphere_point_drawable(kCenters, kRadii, kColors);
    render_one_frame(*instance);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(SphereImpostorRendererTest, DrawWithTranslucentSphereDoesNotCrash) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    instance->add_sphere_point_drawable(kCenters, kRadii, kTranslucentColors);
    render_one_frame(*instance);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(SphereImpostorRendererTest, DrawWithMixedOpacitySphereDoesNotCrash) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    const std::vector<float> mixedColors{1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 0.5F};
    instance->add_sphere_point_drawable(kCenters, kRadii, mixedColors);
    render_one_frame(*instance);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(SphereImpostorRendererTest, MultisamplingSmoothsProceduralSphereSilhouette) {
    auto instance = create_readback_renderer();
    ASSERT_NE(nullptr, instance);
    if (instance->get_max_msaa_samples() < 2) {
        GTEST_SKIP() << "OpenGL context does not support multisampling";
    }

    const std::vector<float> center{0.0F, 0.0F, 0.0F};
    const std::vector<float> radius{1.0F};
    const std::array<float, 4> red{1.0F, 0.0F, 0.0F, 1.0F};
    ASSERT_TRUE(instance->add_sphere_point_drawable(center, radius, red).is_valid());

    renderer::LightingConfig flatLighting;
    flatLighting.lightColor = {0.0F, 0.0F, 0.0F};
    flatLighting.fillLightColor = {0.0F, 0.0F, 0.0F};
    flatLighting.ambientColor = {1.0F, 1.0F, 1.0F};
    flatLighting.materialAmbient = {1.0F, 1.0F, 1.0F};
    flatLighting.materialDiffuse = {0.0F, 0.0F, 0.0F};
    flatLighting.materialSpecular = {0.0F, 0.0F, 0.0F};

    // Count partially covered red pixels explicitly; single-sample discard produces only
    // background or fully red pixels, whereas per-sample discard resolves intermediate values.
    const auto countPartialRedPixels = [&] {
        instance->begin_frame({0.0F, 0.0F, 0.0F, 1.0F});
        instance->draw(flatLighting);
        instance->end_frame();
        glFinish();

        const auto viewport = instance->scene_viewport();
        std::vector<std::uint8_t> row(static_cast<std::size_t>(viewport.framebuffer.width) * 4U);
        glReadBuffer(GL_FRONT);
        glReadPixels(viewport.framebuffer.x,
                     viewport.framebuffer.y + viewport.framebuffer.height / 2,
                     viewport.framebuffer.width,
                     1,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     row.data());
        std::size_t partial = 0;
        for (std::size_t pixel = 0; pixel < row.size() / 4U; ++pixel) {
            const std::uint8_t r = row[pixel * 4U];
            const std::uint8_t g = row[(pixel * 4U) + 1U];
            const std::uint8_t b = row[(pixel * 4U) + 2U];
            partial += r > 5U && r < 250U && g < 5U && b < 5U ? 1U : 0U;
        }
        return partial;
    };

    const std::size_t singleSamplePartial = countPartialRedPixels();
    instance->set_msaa_samples(std::min(4, instance->get_max_msaa_samples()));
    const std::size_t multisamplePartial = countPartialRedPixels();

    EXPECT_EQ(0U, singleSamplePartial);
    EXPECT_GT(multisamplePartial, singleSamplePartial);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(SphereImpostorRendererTest, ProceduralDrawRestoresSampleShadingState) {
    auto instance = create_readback_renderer();
    ASSERT_NE(nullptr, instance);
    if (instance->get_max_msaa_samples() < 2) {
        GTEST_SKIP() << "OpenGL context does not support multisampling";
    }

    ASSERT_TRUE(instance
                    ->add_sphere_point_drawable(std::vector<float>{0.0F, 0.0F, 0.0F},
                                                std::vector<float>{0.5F},
                                                std::array<float, 4>{1.0F, 0.0F, 0.0F, 1.0F})
                    .is_valid());
    instance->set_msaa_samples(std::min(4, instance->get_max_msaa_samples()));
    instance->begin_frame();
    glDisable(GL_SAMPLE_SHADING);
    glMinSampleShading(0.25F);
    instance->draw();

    GLfloat minimum = 0.0F;
    glGetFloatv(GL_MIN_SAMPLE_SHADING_VALUE, &minimum);
    EXPECT_EQ(GL_FALSE, glIsEnabled(GL_SAMPLE_SHADING));
    EXPECT_FLOAT_EQ(0.25F, minimum);
    instance->end_frame();
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

// ──────────────────────────────────────────────────────────────────────────────
// pick_drawables — sphere is selectable
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(SphereImpostorRendererTest, PickDrawablesResolvesSphereCenteredAtOrigin) {
    auto instance = create_renderer();
    ASSERT_NE(nullptr, instance);

    // A single sphere centered at the origin with a radius large enough to
    // occupy the center of the viewport (camera is placed ~10 units back along z).
    const std::vector<float> center{0.0F, 0.0F, 0.0F};
    const std::vector<float> radius{2.0F};
    const std::vector<float> color{1.0F, 1.0F, 1.0F, 1.0F};
    const renderer::DrawableHandle handle = instance->add_sphere_point_drawable(center, radius, color);
    ASSERT_TRUE(handle.is_valid());

    render_one_frame(*instance);

    const auto sceneViewport = instance->scene_viewport();
    const int cx = sceneViewport.framebuffer.width / 2;
    const int cy = sceneViewport.framebuffer.height / 2;
    const auto results = instance->pick_drawables(cx, cy, 2.0);
    ASSERT_EQ(1U, results.size());
    EXPECT_EQ(renderer::DrawableKind::sphere, results.front().handle.kind);
    EXPECT_EQ(handle.id, results.front().handle.id);
    EXPECT_EQ(handle.rendererInstance, results.front().handle.rendererInstance);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

// ──────────────────────────────────────────────────────────────────────────────
// Scene-color readback — the sphere must actually be visible in the presented frame.
//
// Regression guard for the reversed-Z depth bug: the sphere fragment shader used to
// double-reverse gl_FragDepth (u_projection already encodes reversed-Z), so every
// sphere fragment failed the GL_GREATER depth test and the sphere was rasterized but
// never shown. The other tests only check has_value / GL_NO_ERROR / pick resolution
// and so never sampled scene color — this one does.
// ──────────────────────────────────────────────────────────────────────────────

TEST_F(SphereImpostorRendererTest, OpaqueSphereIsVisibleInPresentedScene) {
    auto instance = create_readback_renderer();
    ASSERT_NE(nullptr, instance);
    GLboolean doubleBuffered = GL_TRUE;
    glGetBooleanv(GL_DOUBLEBUFFER, &doubleBuffered);
    ASSERT_EQ(GL_FALSE, doubleBuffered) << "Front-buffer readback requires a single-buffered context";

    // A single red sphere at the origin, large enough to cover the scene center
    // (the camera frames the scene ~10 units back along z).
    const std::vector<float> center{0.0F, 0.0F, 0.0F};
    const std::vector<float> radius{2.0F};
    const std::vector<float> color{1.0F, 0.0F, 0.0F, 1.0F};
    ASSERT_TRUE(instance->add_sphere_point_drawable(center, radius, color).is_valid());

    // Clear to a non-red background so a depth-culled (invisible) sphere would leave the
    // interior clearly distinguishable from the sphere color.
    instance->begin_frame({0.0F, 0.0F, 0.2F, 1.0F});
    instance->draw();
    instance->end_frame();
    glFinish();

    const auto [x, y] = scene_interior_coordinate(*instance);
    const auto pixel = read_front_pixel(x, y);

    // The sphere is red-lit, so red dominates and it is clearly not the blue background.
    EXPECT_GT(static_cast<int>(pixel[0]), 60) << "sphere center should show red-dominant color, not background";
    EXPECT_GT(static_cast<int>(pixel[0]), static_cast<int>(pixel[2]))
        << "red channel should exceed the blue background channel at the sphere center";
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

// The example scenario: a sphere sitting on a coplanar mesh at z=0. The sphere's front
// surface (z = +radius) is nearer the camera than the mesh, so its manually written
// gl_FragDepth must win the depth test and the sphere must occlude the mesh at the center.
// This guards the depth-write mapping against the mesh's hardware-written depth.
TEST_F(SphereImpostorRendererTest, SphereOccludesCoplanarMeshAtCenter) {
    auto instance = create_readback_renderer();
    ASSERT_NE(nullptr, instance);
    GLboolean doubleBuffered = GL_TRUE;
    glGetBooleanv(GL_DOUBLEBUFFER, &doubleBuffered);
    ASSERT_EQ(GL_FALSE, doubleBuffered) << "Front-buffer readback requires a single-buffered context";

    // Grey quad covering the scene center at z=0 (mirrors examples/example_mesh.cpp).
    const std::vector<float> meshVerts{-1.0F, -1.0F, 0.0F, 1.0F, -1.0F, 0.0F, 1.0F, 1.0F, 0.0F, -1.0F, 1.0F, 0.0F};
    const std::vector<std::uint32_t> meshIndices{0U, 1U, 2U, 0U, 2U, 3U};
    ASSERT_TRUE(
        instance->add_mesh_drawable(meshVerts, meshIndices, std::array<float, 4>{0.5F, 0.5F, 0.5F, 1.0F}).is_valid());

    // A red sphere centered on the quad; its front surface protrudes toward the camera.
    const std::vector<float> center{0.0F, 0.0F, 0.0F};
    const std::vector<float> radius{0.5F};
    const std::vector<float> color{1.0F, 0.0F, 0.0F, 1.0F};
    ASSERT_TRUE(instance->add_sphere_point_drawable(center,
                                                     radius,
                                                     color,
                                                     renderer::SphereStyle{renderer::SphereSizeSpace::World})
                    .is_valid());

    instance->begin_frame({0.0F, 0.0F, 0.2F, 1.0F});
    instance->draw();
    instance->end_frame();
    glFinish();

    const auto [x, y] = scene_interior_coordinate(*instance);
    const auto pixel = read_front_pixel(x, y);

    // The center must show the red sphere, not the grey mesh (which would have r ~= g ~= b).
    EXPECT_GT(static_cast<int>(pixel[0]), static_cast<int>(pixel[1]) + 30)
        << "sphere (red) should occlude the coplanar grey mesh at the center";
    EXPECT_GT(static_cast<int>(pixel[0]), static_cast<int>(pixel[2]) + 30)
        << "sphere (red) should occlude the coplanar grey mesh at the center";
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(SphereImpostorRendererTest, OffAxisSphereIsVisibleAtProjectedPosition) {
    auto instance = create_readback_renderer();
    ASSERT_NE(nullptr, instance);

    renderer::CameraAutoFitSettings autoFit;
    autoFit.enabled = false;
    instance->set_camera_auto_fit_settings(autoFit);
    auto camera = instance->get_camera().lock();
    ASSERT_NE(nullptr, camera);
    camera->look_at({0.0, 0.0, 5.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});

    const std::vector<float> center{0.6F, 0.0F, 0.0F};
    const std::vector<float> radius{0.3F};
    const std::array<float, 4> red{0.8F, 0.1F, 0.1F, 1.0F};
    ASSERT_TRUE(instance->add_sphere_point_drawable(center,
                                                     radius,
                                                     red,
                                                     renderer::SphereStyle{renderer::SphereSizeSpace::World})
                    .is_valid());

    instance->begin_frame({0.0F, 0.0F, 0.2F, 1.0F});
    instance->draw();
    instance->end_frame();
    glFinish();

    const auto sv = instance->scene_viewport();
    const int w = sv.framebuffer.width;
    const int h = sv.framebuffer.height;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U);
    glReadBuffer(GL_FRONT);
    glReadPixels(sv.framebuffer.x, sv.framebuffer.y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    int redCount = 0;
    int redXSum = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
                                    static_cast<std::size_t>(x)) *
                                   4U;
            const int r = pixels[i];
            const int g = pixels[i + 1U];
            const int b = pixels[i + 2U];
            if (r > g + 25 && r > b + 25) {
                ++redCount;
                redXSum += x;
            }
        }
    }

    ASSERT_GT(redCount, 100);
    EXPECT_GT(redXSum / redCount, w / 2 + 20);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(SphereImpostorRendererTest, PerspectiveBillboardContainsLargeSphereSilhouette) {
    auto instance = create_readback_renderer(1024U, 1024U);
    ASSERT_NE(nullptr, instance);

    renderer::CameraAutoFitSettings autoFit;
    autoFit.enabled = false;
    instance->set_camera_auto_fit_settings(autoFit);
    auto camera = instance->get_camera().lock();
    ASSERT_NE(nullptr, camera);
    camera->look_at({0.0, 0.0, 5.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});

    constexpr float sphereRadius = 1.28F;
    const std::vector<float> center{0.0F, 0.0F, 0.0F};
    const std::vector<float> radius{sphereRadius};
    const std::array<float, 4> red{0.8F, 0.1F, 0.1F, 1.0F};
    const auto handle = instance->add_sphere_point_drawable(center,
                                                           radius,
                                                           red,
                                                           renderer::SphereStyle{renderer::SphereSizeSpace::World});
    ASSERT_TRUE(handle.is_valid());
    render_one_frame(*instance);

    const auto sv = instance->scene_viewport();
    const double distance = linal::length(camera->get_position() - camera->get_target());
    const double projectionScale = static_cast<double>(camera->get_projection_matrix()(0, 0));
    const double tangentExtent =
        projectionScale * static_cast<double>(sphereRadius) /
        std::sqrt(distance * distance - static_cast<double>(sphereRadius) * static_cast<double>(sphereRadius));
    const double planarExtent = projectionScale * static_cast<double>(sphereRadius) / distance;
    const double tangentLeft = (1.0 - tangentExtent) * sv.framebuffer.width * 0.5;
    const double planarLeft = (1.0 - planarExtent) * sv.framebuffer.width * 0.5;
    const double probeX = std::midpoint(tangentLeft, planarLeft);

    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(sv.framebuffer.width) *
                                     static_cast<std::size_t>(sv.framebuffer.height) * 4U);
    glReadBuffer(GL_FRONT);
    glReadPixels(sv.framebuffer.x,
                 sv.framebuffer.y,
                 sv.framebuffer.width,
                 sv.framebuffer.height,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 pixels.data());
    int minRedX = sv.framebuffer.width;
    for (int y = 0; y < sv.framebuffer.height; ++y) {
        for (int x = 0; x < sv.framebuffer.width; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(sv.framebuffer.width) +
                                    static_cast<std::size_t>(x)) *
                                   4U;
            if (pixels[i] > pixels[i + 1U] + 10 && pixels[i] > pixels[i + 2U] + 10) {
                minRedX = std::min(minRedX, x);
            }
        }
    }
    EXPECT_LE(minRedX, probeX);

    // This point is inside the true tangent silhouette but outside the old center-depth
    // center +/- radius proxy. Picking it therefore guards against a clipped billboard.
    const auto centerPicks = instance->pick_drawables(sv.framebuffer.width * 0.5, sv.framebuffer.height * 0.5, 0.0);
    ASSERT_EQ(1U, centerPicks.size());
    const auto picks = instance->pick_drawables(probeX, sv.framebuffer.height * 0.5, 0.0);
    ASSERT_EQ(1U, picks.size());
    EXPECT_EQ(handle.id, picks.front().handle.id);
    EXPECT_EQ(renderer::DrawableKind::sphere, picks.front().handle.kind);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(SphereImpostorRendererTest, MultipleOffAxisSphereInstancesAreVisible) {
    auto instance = create_readback_renderer();
    ASSERT_NE(nullptr, instance);

    renderer::CameraAutoFitSettings autoFit;
    autoFit.enabled = false;
    instance->set_camera_auto_fit_settings(autoFit);
    auto camera = instance->get_camera().lock();
    ASSERT_NE(nullptr, camera);
    camera->look_at({0.0, 0.0, 5.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});

    const std::vector<float> centers{-0.8F, 0.0F, 0.0F, 0.8F, 0.0F, 0.0F};
    const std::vector<float> radii{0.3F, 0.3F};
    const std::vector<float> colors{0.8F, 0.1F, 0.1F, 1.0F, 0.1F, 0.8F, 0.1F, 1.0F};
    ASSERT_TRUE(instance->add_sphere_point_drawable(centers,
                                                     radii,
                                                     colors,
                                                     renderer::SphereStyle{renderer::SphereSizeSpace::World})
                    .is_valid());

    instance->begin_frame({0.0F, 0.0F, 0.2F, 1.0F});
    instance->draw();
    instance->end_frame();
    glFinish();

    const auto sv = instance->scene_viewport();
    const int w = sv.framebuffer.width;
    const int h = sv.framebuffer.height;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U);
    glReadBuffer(GL_FRONT);
    glReadPixels(sv.framebuffer.x, sv.framebuffer.y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    int leftRed = 0;
    int rightGreen = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
                                    static_cast<std::size_t>(x)) *
                                   4U;
            const int r = pixels[i];
            const int g = pixels[i + 1U];
            const int b = pixels[i + 2U];
            leftRed += x < w / 2 && r > g + 25 && r > b + 25 ? 1 : 0;
            rightGreen += x > w / 2 && g > r + 25 && g > b + 25 ? 1 : 0;
        }
    }

    EXPECT_GT(leftRed, 100);
    EXPECT_GT(rightGreen, 100);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(SphereImpostorRendererTest, OffAxisSphereIsVisibleAndPickableWithOrthographicProjection) {
    auto instance = create_readback_renderer();
    ASSERT_NE(nullptr, instance);

    renderer::CameraAutoFitSettings autoFit;
    autoFit.enabled = false;
    instance->set_camera_auto_fit_settings(autoFit);
    auto camera = instance->get_camera().lock();
    ASSERT_NE(nullptr, camera);
    camera->set_projection_type(renderer::CameraProjectionType::ORTHOGRAPHIC);
    camera->look_at({0.0, 0.0, 5.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});

    const std::vector<float> center{1.0F, 0.0F, 0.0F};
    const std::vector<float> radius{0.5F};
    const std::array<float, 4> red{0.8F, 0.1F, 0.1F, 1.0F};
    const auto handle = instance->add_sphere_point_drawable(center,
                                                           radius,
                                                           red,
                                                           renderer::SphereStyle{renderer::SphereSizeSpace::World});
    ASSERT_TRUE(handle.is_valid());

    render_one_frame(*instance);

    const auto sv = instance->scene_viewport();
    const int expectedX = sv.framebuffer.width * 3 / 5;
    const int expectedY = sv.framebuffer.height / 2;
    const auto pixel = read_front_pixel(sv.framebuffer.x + expectedX, sv.framebuffer.y + expectedY);
    EXPECT_GT(static_cast<int>(pixel[0]), static_cast<int>(pixel[1]) + 25);
    EXPECT_GT(static_cast<int>(pixel[0]), static_cast<int>(pixel[2]) + 25);

    const auto picks = instance->pick_drawables(expectedX, expectedY, 2.0);
    ASSERT_EQ(1U, picks.size());
    EXPECT_EQ(handle.id, picks.front().handle.id);
    EXPECT_EQ(renderer::DrawableKind::sphere, picks.front().handle.kind);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(SphereImpostorRendererTest, NonUniformTransformScalesRenderedAndPickedSphere) {
    auto instance = create_readback_renderer();
    ASSERT_NE(nullptr, instance);

    renderer::CameraAutoFitSettings autoFit;
    autoFit.enabled = false;
    instance->set_camera_auto_fit_settings(autoFit);
    auto camera = instance->get_camera().lock();
    ASSERT_NE(nullptr, camera);
    camera->set_projection_type(renderer::CameraProjectionType::ORTHOGRAPHIC);
    camera->look_at({0.0, 0.0, 5.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});

    const auto handle = instance->add_sphere_point_drawable(std::vector<float>{0.0F, 0.0F, 0.0F},
                                                             std::vector<float>{0.5F},
                                                             std::array<float, 4>{0.8F, 0.1F, 0.1F, 1.0F},
                                                             renderer::SphereStyle{renderer::SphereSizeSpace::World});
    ASSERT_TRUE(handle.is_valid());
    linal::hmatf transform = linal::hmatf::identity();
    transform(0, 0) = 3.0F;
    ASSERT_TRUE(instance->set_drawable_transform(handle, transform));

    render_one_frame(*instance);

    const auto viewport = instance->scene_viewport();
    constexpr int scaledOnlyOffset = 20;
    const int probeX = viewport.framebuffer.x + viewport.framebuffer.width / 2 + scaledOnlyOffset;
    const int probeY = viewport.framebuffer.y + viewport.framebuffer.height / 2;
    const auto pixel = read_front_pixel(probeX, probeY);
    EXPECT_GT(static_cast<int>(pixel[0]), static_cast<int>(pixel[1]) + 25);

    const auto picks = instance->pick_drawables(viewport.framebuffer.width / 2 + scaledOnlyOffset,
                                                viewport.framebuffer.height / 2,
                                                0.0);
    ASSERT_EQ(1U, picks.size());
    EXPECT_EQ(handle.id, picks.front().handle.id);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(SphereImpostorRendererTest, CameraAlignedLightingIsStableAcrossViewRotation) {
    auto instance = create_readback_renderer();
    ASSERT_NE(nullptr, instance);

    renderer::CameraAutoFitSettings autoFit;
    autoFit.enabled = false;
    instance->set_camera_auto_fit_settings(autoFit);
    auto camera = instance->get_camera().lock();
    ASSERT_NE(nullptr, camera);

    ASSERT_TRUE(instance
                    ->add_sphere_point_drawable(std::vector<float>{0.0F, 0.0F, 0.0F},
                                                std::vector<float>{1.0F},
                                                std::array<float, 4>{1.0F, 0.0F, 0.0F, 1.0F})
                    .is_valid());

    renderer::LightingConfig lighting;
    lighting.ambientColor = {0.0F, 0.0F, 0.0F};
    lighting.fillLightColor = {0.0F, 0.0F, 0.0F};
    lighting.materialDiffuse = {1.0F, 1.0F, 1.0F};
    lighting.materialSpecular = {0.0F, 0.0F, 0.0F};

    const auto render_center = [&] {
        instance->begin_frame({0.0F, 0.0F, 0.0F, 1.0F});
        instance->draw(lighting);
        instance->end_frame();
        glFinish();
        const auto [x, y] = scene_interior_coordinate(*instance);
        return read_front_pixel(x, y);
    };

    camera->look_at({0.0, 0.0, 5.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});
    const auto fromZ = render_center();
    camera->look_at({5.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0});
    const auto fromX = render_center();

    EXPECT_GT(fromZ[0], 200U);
    EXPECT_GT(fromX[0], 200U);
    EXPECT_LE(std::abs(static_cast<int>(fromZ[0]) - static_cast<int>(fromX[0])), 5);
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

TEST_F(SphereImpostorRendererTest, TranslucentInstancesSortAfterApplyingDrawableTransform) {
    auto instance = create_readback_renderer();
    ASSERT_NE(nullptr, instance);

    renderer::CameraAutoFitSettings autoFit;
    autoFit.enabled = false;
    instance->set_camera_auto_fit_settings(autoFit);
    auto camera = instance->get_camera().lock();
    ASSERT_NE(nullptr, camera);
    camera->look_at({0.0, 0.0, 5.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});

    const std::vector<float> centers{0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 2.0F};
    const std::vector<float> radii{1.5F, 1.5F};
    const std::vector<float> colors{1.0F, 0.0F, 0.0F, 0.5F, 0.0F, 1.0F, 0.0F, 0.5F};
    const auto handle = instance->add_sphere_point_drawable(centers, radii, colors);
    ASSERT_TRUE(handle.is_valid());
    linal::hmatf transform = linal::hmatf::identity();
    transform(2, 2) = -1.0F;
    ASSERT_TRUE(instance->set_drawable_transform(handle, transform));

    renderer::LightingConfig flatLighting;
    flatLighting.lightColor = {0.0F, 0.0F, 0.0F};
    flatLighting.fillLightColor = {0.0F, 0.0F, 0.0F};
    flatLighting.ambientColor = {1.0F, 1.0F, 1.0F};
    flatLighting.materialAmbient = {1.0F, 1.0F, 1.0F};
    flatLighting.materialDiffuse = {0.0F, 0.0F, 0.0F};
    flatLighting.materialSpecular = {0.0F, 0.0F, 0.0F};

    instance->begin_frame({0.0F, 0.0F, 0.0F, 1.0F});
    instance->draw(flatLighting);
    instance->end_frame();
    glFinish();

    const auto [x, y] = scene_interior_coordinate(*instance);
    const auto pixel = read_front_pixel(x, y);
    EXPECT_GT(static_cast<int>(pixel[0]), static_cast<int>(pixel[1]) + 20)
        << "the nearer red sphere must blend after the transformed farther green sphere";
    EXPECT_EQ(GL_NO_ERROR, glGetError());
}

} // namespace
