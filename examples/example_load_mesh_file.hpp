#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include "plinth/Renderer.hpp"
#include "plinth/Texture.hpp"
#include "plinth/WindowSettings.hpp"
#include "plinth/loader/MeshData.hpp"
#include "plinth/loader/MeshLoader.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <span>
#include <stb_image.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mesh_example {
inline renderer::TextureHandle load_texture_cached(renderer::Renderer& renderer,
                                                   std::unordered_map<std::string, renderer::TextureHandle>& cache,
                                                   const std::string& path) {
    if (path.empty()) {
        return {};
    }
    if (const auto it = cache.find(path); it != cache.end()) {
        return it->second;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (pixels == nullptr) {
        std::cerr << "Warning: could not decode diffuse texture '" << path << "'";
        if (const char* reason = stbi_failure_reason(); reason != nullptr) {
            std::cerr << ": " << reason;
        }
        std::cerr << ". Falling back to the material's Kd color.\n";
        cache.emplace(path, renderer::TextureHandle{});
        return {};
    }

    const std::span<const std::uint8_t> rgba8{pixels,
                                              static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U};
    const renderer::TextureHandle handle = renderer.create_texture_2d({static_cast<std::uint32_t>(width),
                                                                       static_cast<std::uint32_t>(height),
                                                                       rgba8,
                                                                       renderer::TextureColorSpace::srgb,
                                                                       renderer::TextureFilter::linear});
    stbi_image_free(pixels);
    if (!handle.is_valid()) {
        std::cerr << "Warning: decoded diffuse texture '" << path
                  << "', but the renderer could not upload it. Falling back to the material's Kd color.\n";
    }
    cache.emplace(path, handle);
    return handle;
}

inline const renderer::MeshData::Material* find_material(const renderer::MeshData& mesh,
                                                         const std::string& materialName) {
    for (const renderer::MeshData::Material& material: mesh.materials) {
        if (material.name == materialName) {
            return &material;
        }
    }
    return nullptr;
}

inline renderer::DrawableHandle add_colored_submesh(renderer::Renderer& renderer,
                                                    const renderer::MeshData& mesh,
                                                    std::span<const std::uint32_t> indices,
                                                    std::array<float, 4> color) {
    if (mesh.normals.size() == mesh.vertices.size()) {
        return renderer.add_mesh_drawable(mesh.vertices, indices, color, mesh.normals);
    }
    return renderer.add_mesh_drawable(mesh.vertices, indices, color);
}

inline bool add_mesh(renderer::Renderer& renderer, const renderer::MeshData& mesh) {
    const std::array<float, 4> defaultColor{0.8F, 0.8F, 0.8F, 1.0F};
    if (mesh.subMeshes.empty()) {
        return renderer.add_mesh_drawable(mesh, defaultColor).is_valid();
    }

    const std::size_t vertexCount = mesh.vertices.size() / 3U;
    std::vector<float> whiteColors(vertexCount * 4U, 1.0F);
    std::unordered_map<std::string, renderer::TextureHandle> textureCache;

    for (const renderer::MeshData::SubMesh& sub: mesh.subMeshes) {
        const std::span<const std::uint32_t> subIndices{mesh.triangleIndices.data() + sub.indexOffset, sub.indexCount};
        const renderer::MeshData::Material* material = find_material(mesh, sub.materialName);
        if (material == nullptr) {
            std::cerr << "Warning: OBJ material '" << sub.materialName << "' was not found; using default gray.\n";
        }

        renderer::TextureHandle texture;
        const bool canUseTexture = material != nullptr && !material->diffuseTexturePath.empty() &&
                                   mesh.textureCoordinates.size() == vertexCount * 2U &&
                                   mesh.normals.size() == mesh.vertices.size();
        if (canUseTexture) {
            texture = load_texture_cached(renderer, textureCache, material->diffuseTexturePath);
        } else if (material != nullptr && !material->diffuseTexturePath.empty()) {
            std::cerr << "Warning: material '" << material->name
                      << "' has a diffuse texture, but the mesh has invalid UVs or normals; using its Kd color.\n";
        }

        renderer::DrawableHandle drawable;
        if (texture.is_valid()) {
            drawable = renderer.add_textured_mesh_drawable(mesh.vertices,
                                                           mesh.normals,
                                                           mesh.textureCoordinates,
                                                           whiteColors,
                                                           subIndices,
                                                           texture);
        } else {
            const std::array<float, 4> color = material == nullptr ? defaultColor : material->diffuseColor;
            drawable = add_colored_submesh(renderer, mesh, subIndices, color);
        }
        if (!drawable.is_valid()) {
            std::cerr << "Error: failed to create drawable for material '" << sub.materialName << "'.\n";
            return false;
        }
    }
    return true;
}

inline std::string_view load_error_message(renderer::LoadError error) {
    switch (error) {
    case renderer::LoadError::fileNotFound:      return "file not found or path is not a regular file";
    case renderer::LoadError::unreadable:        return "file exists but could not be read";
    case renderer::LoadError::unsupportedFormat: return "unsupported mesh file extension";
    case renderer::LoadError::parseError:        return "mesh contents are malformed";
    case renderer::LoadError::empty:             return "mesh contains no geometry";
    }
    return "unknown loader error";
}

inline int run(const char* title, const std::filesystem::path& meshPath) {
    renderer::WindowSettings settings;
    settings.title = title;
    settings.width = 1024;
    settings.height = 768;

    auto renderer = renderer::Renderer::create(settings);
    if (!renderer) {
        std::cerr << "Error: renderer initialization failed.\n";
        return 1;
    }

    const renderer::MeshLoadOptions options{.shading = renderer::ShadingMode::Preserve};
    auto mesh = renderer::load_mesh(meshPath, options);
    if (!mesh) {
        std::cerr << "Error: failed to load mesh '" << meshPath.string() << "': " << load_error_message(mesh.error())
                  << ".\n";
        return 2;
    }

    if (!mesh->materialLibrary.empty() && mesh->materials.empty()) {
        const std::filesystem::path materialPath = meshPath.parent_path() / mesh->materialLibrary;
        std::cerr << "Warning: OBJ references material library '" << materialPath.string()
                  << "', but no materials could be loaded.\n";
    }

    stbi_set_flip_vertically_on_load(1);
    if (!add_mesh(*renderer, *mesh)) {
        return 3;
    }

    while (!renderer->should_close()) {
        renderer::Renderer::poll_events();
        if (renderer->is_escape_pressed()) {
            break;
        }
        renderer->begin_frame();
        renderer->draw();
        renderer->end_frame();
    }
    return 0;
}
} // namespace mesh_example
