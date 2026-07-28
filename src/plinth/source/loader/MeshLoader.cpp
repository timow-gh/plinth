#include "plinth/loader/MeshLoader.hpp"
#include "plinth/loader/MeshFormatLoader.hpp"
#include "plinth/loader/MeshShading.hpp"
#include "plinth/loader/MtlLoader.hpp"
#include "plinth/loader/ObjLoader.hpp"
#include "plinth/loader/StlLoader.hpp"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <vector>

namespace renderer {

namespace {

using BuiltinMeshLoaders = MeshLoaderList<ObjLoader, StlLoader>;

// Conventional up-axis per format, used when the caller does not set one. OBJ is
// authored Y-up in the Wavefront ecosystem; STL is Z-up in the CAD world.
[[nodiscard]]
SourceUpAxis conventional_up_axis(std::string_view extension) {
    std::string lowered(extension);
    for (char& c: lowered) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return lowered == ".obj" ? SourceUpAxis::Y : SourceUpAxis::Z;
}

// Rotates every xyz triplet in \p buffer by +90 degrees about the X axis so that
// a Y-up source becomes Z-up: (x, y, z) -> (x, -z, y). Applied to both positions
// and normals; for a pure rotation the normal transform equals the position one.
void rotate_y_up_to_z_up(std::vector<float>& buffer) {
    for (std::size_t i = 0; i + 2 < buffer.size(); i += 3) {
        const float y = buffer[i + 1];
        const float z = buffer[i + 2];
        buffer[i + 1] = -z;
        buffer[i + 2] = y;
    }
}

// Converts parsed geometry from its source up-axis into the renderer's Z-up
// convention, in place. A no-op when the source is already Z-up.
void apply_up_axis(MeshData& mesh, SourceUpAxis upAxis) {
    if (upAxis == SourceUpAxis::Z) {
        return;
    }
    rotate_y_up_to_z_up(mesh.vertices);
    rotate_y_up_to_z_up(mesh.normals);
}

[[nodiscard]]
std::expected<std::string, LoadError> read_file(const std::filesystem::path& path) {
    std::error_code errorCode;
    if (!std::filesystem::is_regular_file(path, errorCode) || errorCode) {
        return std::unexpected(LoadError::fileNotFound);
    }
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return std::unexpected(LoadError::unreadable);
    }
    std::string contents(static_cast<std::size_t>(std::filesystem::file_size(path, errorCode)), '\0');
    if (errorCode) {
        return std::unexpected(LoadError::unreadable);
    }
    stream.read(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (stream.bad()) {
        return std::unexpected(LoadError::unreadable);
    }
    return contents;
}

// Flat/Smooth shading rebuilds the vertex arrays and would invalidate texture
// coordinates and submesh index ranges. A mesh carrying UVs is meant to be
// rendered textured (via ShadingMode::Preserve), so re-shading is skipped for it
// and the parsed normals/UVs/submeshes are kept intact. A mesh with submeshes
// but no UVs can still be re-shaded, but its per-material index ranges no longer
// map onto the rebuilt geometry, so the (now meaningless) grouping is dropped.
[[nodiscard]]
std::expected<MeshData, LoadError> apply_shading_if_supported(const MeshData& mesh, ShadingMode mode) {
    if (mode == ShadingMode::Preserve || !mesh.textureCoordinates.empty()) {
        return mesh;
    }
    MeshData shaded = apply_shading(mesh, mode);
    shaded.subMeshes.clear();
    return shaded;
}

// Resolves an OBJ's recorded material library against the source directory and
// fills each submesh's material into mesh.materials with a diffuse texture path.
// A missing or unparseable .mtl is non-fatal: the mesh still loads, untextured.
void resolve_materials(MeshData& mesh, const std::filesystem::path& sourcePath) {
    if (mesh.materialLibrary.empty()) {
        return;
    }
    const std::filesystem::path baseDir = sourcePath.parent_path();
    const std::filesystem::path mtlPath = baseDir / mesh.materialLibrary;
    auto contents = read_file(mtlPath);
    if (!contents) {
        return;
    }
    auto materials = parse_mtl(*contents, baseDir);
    if (!materials) {
        return;
    }
    mesh.materials = std::move(*materials);
}

} // namespace

std::expected<MeshData, LoadError>
load_mesh(std::string_view extension, std::string contents, MeshLoadOptions options) {
    auto mesh = load_mesh_with(BuiltinMeshLoaders{}, extension, contents);
    if (mesh) {
        mesh = apply_shading_if_supported(*mesh, options.shading);
    }
    if (mesh) {
        apply_up_axis(*mesh, options.upAxis.value_or(conventional_up_axis(extension)));
    }
    return mesh;
}

std::expected<MeshData, LoadError> load_mesh(const std::filesystem::path& path, MeshLoadOptions options) {
    auto contents = read_file(path);
    if (!contents) {
        return std::unexpected(contents.error());
    }
    const std::string extension = path.extension().string();
    auto mesh = load_mesh_with(BuiltinMeshLoaders{}, extension, *contents);
    if (mesh) {
        mesh = apply_shading_if_supported(*mesh, options.shading);
    }
    if (mesh) {
        apply_up_axis(*mesh, options.upAxis.value_or(conventional_up_axis(extension)));
        // Resolve materials before overwriting sourceName, using the full path's
        // directory to locate the .mtl referenced by the OBJ.
        resolve_materials(*mesh, path);
        mesh->sourceName = path.filename().string();
    }
    return mesh;
}

} // namespace renderer
