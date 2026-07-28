#include "plinth/loader/MeshLoader.hpp"
#include "plinth/loader/MeshFormatLoader.hpp"
#include "plinth/loader/MeshShading.hpp"
#include "plinth/loader/ObjLoader.hpp"
#include "plinth/loader/StlLoader.hpp"
#include <fstream>
#include <ios>
#include <string>
#include <string_view>

namespace renderer {

namespace {

using BuiltinMeshLoaders = MeshLoaderList<ObjLoader, StlLoader>;

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

} // namespace

std::expected<MeshData, LoadError>
load_mesh(std::string_view extension, std::string contents, MeshLoadOptions options) {
    auto mesh = load_mesh_with(BuiltinMeshLoaders{}, extension, contents);
    if (mesh) {
        mesh = apply_shading(*mesh, options.shading);
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
        mesh = apply_shading(*mesh, options.shading);
        mesh->sourceName = path.filename().string();
    }
    return mesh;
}

} // namespace renderer
