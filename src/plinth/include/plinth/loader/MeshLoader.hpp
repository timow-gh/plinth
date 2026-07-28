#ifndef RENDERER_LOADER_MESHLOADER_HPP
#define RENDERER_LOADER_MESHLOADER_HPP

#include "plinth/loader/MeshData.hpp"
#include "plinth/plinth_export.h"
#include <expected>
#include <filesystem>
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

/// Options controlling how a mesh is post-processed after parsing. Defaults keep
/// the raw imported data unchanged.
struct MeshLoadOptions {
    ShadingMode shading{ShadingMode::Preserve};
};

[[nodiscard]]
PLINTH_EXPORT std::expected<MeshData, LoadError>
load_mesh(const std::filesystem::path& path, MeshLoadOptions options = {});

/// Parses in-memory file contents, choosing a parser by \p extension (e.g.
/// ".obj"). \p contents holds the raw bytes of the file (text or binary).
[[nodiscard]]
PLINTH_EXPORT std::expected<MeshData, LoadError>
load_mesh(std::string_view extension, std::string contents, MeshLoadOptions options = {});

} // namespace renderer

#endif // RENDERER_LOADER_MESHLOADER_HPP
