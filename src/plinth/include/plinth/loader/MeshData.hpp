#ifndef RENDERER_LOADER_MESHDATA_HPP
#define RENDERER_LOADER_MESHDATA_HPP

#include "plinth/plinth_export.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace renderer {

struct PLINTH_EXPORT MeshData {
    /// A contiguous run of triangleIndices that shares one material. Formats that
    /// carry no material information leave subMeshes empty; the whole mesh is then
    /// a single implicit group.
    struct SubMesh {
        std::uint32_t indexOffset{}; // offset into triangleIndices
        std::uint32_t indexCount{};  // number of indices in this submesh
        std::string materialName;    // raw usemtl name ("" = default/none)
    };

    /// A material referenced by name. diffuseColor comes from the .mtl Kd
    /// directive and defaults to opaque white. diffuseTexturePath is resolved
    /// (relative to the source file's directory) from map_Kd; it is empty when
    /// the material has no diffuse texture or none could be resolved.
    struct Material {
        std::string name;
        std::array<float, 4> diffuseColor{1.0F, 1.0F, 1.0F, 1.0F};
        std::string diffuseTexturePath;
    };

    std::vector<float> vertices;
    std::vector<std::uint32_t> triangleIndices;
    std::vector<float> normals;
    std::vector<float> colors;
    /// Two floats (u, v) per vertex, parallel to vertices. Empty when the source
    /// carried no texture coordinates.
    std::vector<float> textureCoordinates;
    std::vector<SubMesh> subMeshes;
    std::vector<Material> materials;
    /// Name of the material library (.mtl) referenced by the source (OBJ mtllib),
    /// relative to the source file's directory. Empty when none was declared. The
    /// pure-string parser records this; the path-aware loader resolves it into
    /// materials.
    std::string materialLibrary;
    std::string sourceName;

    [[nodiscard]] bool empty() const { return vertices.empty(); }
};

} // namespace renderer

#endif // RENDERER_LOADER_MESHDATA_HPP
