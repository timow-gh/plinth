#ifndef PLINTH_UIMODE_HPP
#define PLINTH_UIMODE_HPP

namespace renderer {

/// Selects which control surface the renderer's ImGui overlay presents.
/// Debug exposes the full set of post-processing and visualization controls;
/// Release exposes a small, game-like set (quality preset, exposure, fog toggle)
/// with debug-only state pinned to sensible defaults.
enum class UiMode { Debug = 0, Release = 1 };

} // namespace renderer

#endif // PLINTH_UIMODE_HPP
