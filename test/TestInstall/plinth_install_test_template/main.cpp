#include <plinth/Renderer.hpp>

int main() {
    renderer::ClearColor clearColor{0.05F, 0.05F, 0.08F, 1.0F};
    renderer::LineType lineType = renderer::LineType::lines();
    (void)lineType.get_gl_type();
    renderer::BufferAccessPattern pattern = renderer::BufferAccessPattern::STATIC_DRAW;
    (void)pattern;
    renderer::LightingConfig lighting;
    (void)lighting.lightPosition[0];
    renderer::SceneViewport viewport =
        renderer::Renderer::calculate_scene_viewport({800, 600}, {800, 600}, 0.0);
    (void)viewport.framebuffer.width;
    return 0;
}
