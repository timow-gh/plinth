#ifndef RENDERER_LOADER_MTLLOADER_HPP
#define RENDERER_LOADER_MTLLOADER_HPP

#include "plinth/loader/MeshData.hpp"
#include "plinth/loader/MeshLoader.hpp"
#include "plinth/plinth_export.h"

#include <expected>
#include <filesystem>
#include <string_view>
#include <vector>

namespace renderer {

/// Parses a Wavefront material library (.mtl). Each `newmtl` block becomes a
/// MeshData::Material. Its `Kd` diffuse color is retained, and its `map_Kd`
/// diffuse texture is resolved against
/// \p baseDir (the directory containing the .mtl file) and normalized so
/// backslash-separated, relative paths from Windows/Blender exports become a
/// usable filesystem path. Other directives (Ka/Ks/Ns/map_Bump/...) are ignored
/// for now. A material with no map_Kd yields an empty diffuseTexturePath.
///
/// Parsing is lenient: malformed or unknown lines are skipped rather than
/// rejected. Returns LoadError::empty when no material was declared.
[[nodiscard]] PLINTH_EXPORT std::expected<std::vector<MeshData::Material>, LoadError>
parse_mtl(std::string_view contents, const std::filesystem::path& baseDir);

} // namespace renderer

#endif // RENDERER_LOADER_MTLLOADER_HPP
