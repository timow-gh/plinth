#ifndef PLINTH_STROKESTYLE_HPP
#define PLINTH_STROKESTYLE_HPP

#include "plinth/DashSpace.hpp"

#include <vector>

namespace renderer {

/// How a line endpoint is terminated.
enum class LineCap {
    Butt,   ///< Flush with the endpoint — no extension. SVG default.
    Square, ///< Extends half the line width past the endpoint (flat).
    Round,  ///< Semicircle centered on the endpoint.
};

/// How the junction between two connected segments is rendered.
enum class LineJoin {
    Miter, ///< Sharp pointed join extended to intersection. SVG default.
    Bevel, ///< Diagonal cutoff at the join.
    Round, ///< Circular arc join centered on the shared vertex.
};

/// Stroke styling parameters.
///
/// Groups all per-drawable style properties that apply uniformly to a line drawable.
/// Maps 1:1 to SVG stroke properties to enable future SVG export.
///
/// Does NOT include:
///   - color           (per-vertex, supplied separately)
///   - topology        (LineType)
///   - buffer hints    (BufferAccessPattern)
///   - per-vertex data (perVertexDashFlags)
struct StrokeStyle {
    float lineWidth{2.0F};
    LineCap cap{LineCap::Butt};
    LineJoin join{LineJoin::Miter};
    /// Clamps the miter spike length. SVG default 4. Only relevant when join == Miter.
    float miterLimit{4.0F};

    /// Dash pattern: alternating on/off lengths (SVG stroke-dasharray semantics).
    /// Even indices [0,2,4,...] are dash lengths; odd indices [1,3,5,...] are gap lengths.
    /// Empty vector means solid (no dashing). {dashLen, gapLen} is the common single-pair case.
    std::vector<float> dashPattern{};
    /// Offsets the start of the pattern along the arc length (marching ants).
    float dashPhase{0.0F};
    DashSpace dashSpace{DashSpace::World};
};

/// Convenience factory: single dash+gap pair.
[[nodiscard]] inline StrokeStyle
make_dashed_stroke(float lineWidth, float dashLength, float gapLength, DashSpace space = DashSpace::World) {
    return StrokeStyle{lineWidth, LineCap::Butt, LineJoin::Miter, 4.0F, {dashLength, gapLength}, 0.0F, space};
}

} // namespace renderer

#endif // PLINTH_STROKESTYLE_HPP
