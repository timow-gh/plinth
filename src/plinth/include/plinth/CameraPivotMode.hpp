#ifndef RENDERER_CAMERAPIVOTMODE_HPP
#define RENDERER_CAMERAPIVOTMODE_HPP

namespace renderer {

/// Selects what point right-drag rotation pivots around.
/// GROUND_CURSOR rotates around where the ray through the mouse cursor hits the
/// ground plane (captured at drag start) and allows panning; ORIGIN rotates
/// around the world origin (0,0,0), disallows panning, and still allows scroll/zoom.
enum class CameraPivotMode {
    GROUND_CURSOR = 0,
    ORIGIN
};

} // namespace renderer

#endif // RENDERER_CAMERAPIVOTMODE_HPP
