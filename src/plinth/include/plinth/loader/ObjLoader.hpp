#ifndef RENDERER_LOADER_OBJLOADER_HPP
#define RENDERER_LOADER_OBJLOADER_HPP

#include "plinth/loader/MeshData.hpp"
#include "plinth/loader/MeshLoader.hpp"
#include <array>
#include <expected>
#include <string_view>

namespace renderer {

/// Parser for Wavefront OBJ (ASCII). Handles v/vn positions and normals, faces
/// with any of the `v`, `v//vn`, `v/vt`, `v/vt/vn` vertex forms, and triangulates
/// convex polygons with a triangle fan. Texture coordinates and material
/// directives are accepted but ignored. Models the MeshFormatLoader concept.
struct ObjLoader {
    [[nodiscard]]
    static constexpr std::array<std::string_view, 1> extensions() {
        return {".obj"};
    }

    [[nodiscard]]
    std::expected<MeshData, LoadError> parse(std::string_view rawContents) const;
};

} // namespace renderer

#endif // RENDERER_LOADER_OBJLOADER_HPP
