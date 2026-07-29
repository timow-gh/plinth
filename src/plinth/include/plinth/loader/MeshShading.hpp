#ifndef RENDERER_LOADER_MESHSHADING_HPP
#define RENDERER_LOADER_MESHSHADING_HPP

#include "plinth/loader/MeshData.hpp"
#include "plinth/loader/MeshLoader.hpp"

namespace renderer {

/// Applies a shading mode to a parsed mesh and returns the result.
///
/// - Preserve: returns \p mesh unchanged.
/// - Flat: rebuilds the mesh so every triangle owns three unshared vertices,
///   each carrying that triangle's geometric face normal. Existing per-vertex
///   normals are discarded. Yields hard edges.
/// - Smooth: welds vertices by position and stores one area-weighted averaged
///   normal per position. Yields soft, interpolated shading.
///
/// A mesh with no triangles (fewer than three indices per triangle) is returned
/// unchanged, since there are no faces to derive normals from.
[[nodiscard]] MeshData apply_shading(const MeshData& mesh, ShadingMode mode);

} // namespace renderer

#endif // RENDERER_LOADER_MESHSHADING_HPP
