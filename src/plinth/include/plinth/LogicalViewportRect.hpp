#ifndef RENDERER_LOGICALVIEWPORTRECT_HPP
#define RENDERER_LOGICALVIEWPORTRECT_HPP

namespace renderer {

/// A rectangle in logical (window) coordinates, used to describe the region the 3D
/// scene occupies within the window. Shared by the Renderer's scene-viewport API and
/// by overlays that report the region they leave free for the scene.
struct LogicalViewportRect {
    double x{0.0};
    double y{0.0};
    double width{1.0};
    double height{1.0};

    [[nodiscard]] bool contains(double xpos, double ypos) const {
        return xpos >= x && ypos >= y && xpos < x + width && ypos < y + height;
    }
};

} // namespace renderer

#endif // RENDERER_LOGICALVIEWPORTRECT_HPP
