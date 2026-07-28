#ifndef RENDERER_LOADER_STLLOADER_HPP
#define RENDERER_LOADER_STLLOADER_HPP

#include "plinth/loader/MeshData.hpp"
#include "plinth/loader/MeshLoader.hpp"
#include <array>
#include <expected>
#include <string_view>

namespace renderer {

/// Parser for STL, auto-detecting binary vs ASCII. STL stores unshared
/// per-triangle vertices, so the result has three vertices per triangle,
/// sequential indices, and per-facet normals replicated to each vertex. Models
/// the MeshFormatLoader concept.
struct StlLoader {
    [[nodiscard]]
    static constexpr std::array<std::string_view, 1> extensions() {
        return {".stl"};
    }

    [[nodiscard]]
    std::expected<MeshData, LoadError> parse(std::string_view rawContents) const;
};

} // namespace renderer

#endif // RENDERER_LOADER_STLLOADER_HPP
