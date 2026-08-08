#ifndef RENDERER_LOADER_MESHLOADER_HPP
#define RENDERER_LOADER_MESHLOADER_HPP

#include "plinth/loader/MeshData.hpp"
#include "plinth/plinth_export.h"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace renderer {

enum class LoadError {
    fileNotFound,      // The path does not exist or is not a regular file.
    unreadable,        // The file exists but could not be opened or read.
    unsupportedFormat, // No registered parser handles the file's extension.
    parseError,        // A parser recognized the format but the contents were malformed.
    empty,             // Parsing succeeded but produced no geometry.
};

/// How normals are treated after a mesh is parsed.
enum class ShadingMode {
    /// Keep whatever the file provided: normals stay as parsed, or are left for
    /// the renderer to compute (smooth) when the file has none. This is the
    /// default and preserves the raw imported data.
    Preserve,
    /// Force flat shading: every triangle gets its own three vertices and a
    /// single geometric face normal. Produces hard edges (correct for faceted
    /// models such as a cube). Discards any per-vertex normals from the file.
    Flat,
    /// Force smooth shading: weld vertices sharing a position and average their
    /// face normals, giving a single interpolated normal per position.
    Smooth,
};

/// Which axis points "up" in a source file's coordinate system. The renderer is
/// Z-up; geometry authored with a different up-axis is rotated into Z-up at load.
enum class SourceUpAxis {
    /// Z is up (matches the renderer; no rotation applied).
    Z,
    /// Y is up (the Wavefront OBJ convention); rotated +90 degrees about X so
    /// model +Y becomes world +Z.
    Y,
};

/// Options controlling how a mesh is post-processed after parsing. Defaults keep
/// the raw imported data unchanged.
struct MeshLoadOptions {
    /// Shading is only applied to meshes without texture coordinates or
    /// submeshes. Flat/Smooth rebuild the vertex arrays and would invalidate a
    /// textured, multi-material mesh's UVs and submesh index ranges, so for such
    /// meshes the requested mode is ignored and the parsed data is preserved.
    ShadingMode shading{ShadingMode::Preserve};
    /// Source up-axis. When unset, each format's conventional default is used
    /// (OBJ is treated as Y-up, STL as Z-up). Set explicitly to override.
    std::optional<SourceUpAxis> upAxis{};
};

[[nodiscard]] PLINTH_EXPORT std::expected<MeshData, LoadError> load_mesh(const std::filesystem::path& path,
                                                                         MeshLoadOptions options = {});

/// Parses in-memory file contents, choosing a parser by \p extension (e.g.
/// ".obj"). \p contents holds the raw bytes of the file (text or binary).
[[nodiscard]] PLINTH_EXPORT std::expected<MeshData, LoadError>
load_mesh(std::string_view extension, const std::string& contents, MeshLoadOptions options = {});

} // namespace renderer

#endif // RENDERER_LOADER_MESHLOADER_HPP
