#ifndef OPENGL_DRAWABLE_DRAWABLESMANAGER_HPP
#define OPENGL_DRAWABLE_DRAWABLESMANAGER_HPP

#include "OpenGL/Drawable/LineDrawable.hpp"
#include "OpenGL/Drawable/MeshDrawable.hpp"
#include "OpenGL/Drawable/PointDrawable.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/PickId.hpp"
#include "OpenGL/Programs/ProgramManager.hpp"
#include "OpenGL/Texture2D.hpp"
#include "plinth/DashSpace.hpp"
#include "plinth/LightingConfig.hpp"
#include "plinth/MeshCullFaceMode.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <linal/hmat.hpp>
#include <linal/vec.hpp>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace opengl {

using renderer::LightingConfig;
using renderer::MeshCullFaceMode;

// Drawable category reported by the pick pass. Kept independent of renderer::DrawableKind so the
// OpenGL layer does not depend on the public Renderer header; Renderer translates it.
enum class PickDrawableKind {
    point,
    line,
    mesh,
};

class DrawablesManager {
  public:
    using DrawableId = std::uint64_t;

    // Maps a pass-index (the value encoded into the pick color, starting at 1) back to the drawable
    // it represents. The vector is indexed by (passIndex - 1).
    struct PickEntry {
        PickDrawableKind kind{};
        DrawableId id{};
    };

  private:
    template <typename Drawable>
    struct DrawableEntry {
        DrawableId id{};
        Drawable drawable;
        linal::hmatf transform{linal::hmatf::identity()};
    };

    struct ScopedDepthMask {
        GLboolean previousValue{GL_TRUE};

        explicit ScopedDepthMask(GLboolean value) {
            glGetBooleanv(GL_DEPTH_WRITEMASK, &previousValue);
            glDepthMask(value);
        }

        ~ScopedDepthMask() { glDepthMask(previousValue); }
    };

    struct ScopedCullFaceDisabled {
        GLboolean wasEnabled{GL_FALSE};

        ScopedCullFaceDisabled()
            : wasEnabled(glIsEnabled(GL_CULL_FACE)) {
            glDisable(GL_CULL_FACE);
        }

        ~ScopedCullFaceDisabled() {
            if (wasEnabled == GL_TRUE) {
                glEnable(GL_CULL_FACE);
            } else {
                glDisable(GL_CULL_FACE);
            }
        }
    };

    ProgramManager programManager;
    DrawableId m_nextDrawableId{1U};
    DrawableId m_nextTextureId{1U};
    std::unordered_map<DrawableId, std::shared_ptr<Texture2D>> m_textures;

    std::vector<DrawableEntry<opengl::PointDrawable>> m_pointDrawables;
    std::vector<DrawableEntry<opengl::LineDrawable>> m_lineDrawables;
    std::vector<DrawableEntry<opengl::MeshDrawable>> m_meshDrawables;

    std::unordered_map<DrawableId, MeshCullFaceMode> m_meshCullModes;

  public:
    DrawablesManager(const DrawablesManager&) = delete;
    DrawablesManager& operator=(const DrawablesManager&) = delete;
    DrawablesManager(DrawablesManager&&) = delete;
    DrawablesManager& operator=(DrawablesManager&&) = delete;
    ~DrawablesManager() { clear_drawables(); }

    [[nodiscard]]
    static std::unique_ptr<DrawablesManager> create() {
        ProgramManager programManager;
        programManager.compile();

        if (!programManager.is_compiled()) {
            return nullptr;
        }

        return std::unique_ptr<DrawablesManager>(new DrawablesManager(std::move(programManager)));
    }

    [[nodiscard]]
    bool has_drawables() const {
        return !m_pointDrawables.empty() || !m_lineDrawables.empty() || !m_meshDrawables.empty();
    }

    [[nodiscard]]
    bool has_point_drawables() const {
        return !m_pointDrawables.empty();
    }
    [[nodiscard]]
    bool has_line_drawables() const {
        return !m_lineDrawables.empty();
    }
    [[nodiscard]]
    bool has_mesh_drawables() const {
        return !m_meshDrawables.empty();
    }

    // Collects a position buffer (world-space xyz triplets, transformed by each drawable's
    // current transform) for every currently-added drawable, for use with
    // renderer::calculate_camera_auto_fit. Unlike the untransformed local vertex data a drawable
    // caches internally, this must return owned data - applying a transform produces new values,
    // not a view into existing memory.
    [[nodiscard]]
    std::vector<std::vector<float>> collect_vertex_position_buffers() const {
        std::vector<std::vector<float>> buffers;
        buffers.reserve(m_pointDrawables.size() + m_lineDrawables.size() + m_meshDrawables.size());
        const auto collect = [&buffers](const auto& drawables) {
            for (const auto& entry: drawables) {
                const auto span = entry.drawable.get_vertex_positions();
                if (span.empty()) {
                    continue;
                }
                std::vector<float> transformed;
                transformed.reserve(span.size());
                for (std::size_t i = 0; i + 2 < span.size(); i += 3) {
                    const linal::float3 transformedPoint = linal::to_vec(
                        entry.transform * linal::to_hvec(linal::float3{span[i], span[i + 1], span[i + 2]}));
                    transformed.push_back(transformedPoint[0]);
                    transformed.push_back(transformedPoint[1]);
                    transformed.push_back(transformedPoint[2]);
                }
                buffers.push_back(std::move(transformed));
            }
        };
        collect(m_pointDrawables);
        collect(m_lineDrawables);
        collect(m_meshDrawables);
        return buffers;
    }

    DrawableId add_point_drawable(opengl::PointDrawable drawable) {
        const DrawableId id = next_drawable_id();
        m_pointDrawables.emplace_back(DrawableEntry<opengl::PointDrawable>{id, std::move(drawable)});
        return id;
    }
    DrawableId add_line_drawable(opengl::LineDrawable drawable) {
        const DrawableId id = next_drawable_id();
        m_lineDrawables.emplace_back(DrawableEntry<opengl::LineDrawable>{id, std::move(drawable)});
        return id;
    }
    std::optional<DrawableId> add_point_drawable(std::span<const float> vertices,
                                                 std::span<const float> colors,
                                                 std::span<const std::uint32_t> indices,
                                                 float pointSize,
                                                 opengl::BufferAccessPattern accessPattern) {
        auto drawable =
            opengl::make_point_drawable(get_point_program(), vertices, 3, colors, 4, indices, pointSize, accessPattern);
        if (!drawable.has_value()) {
            return std::nullopt;
        }

        return add_point_drawable(std::move(drawable.value()));
    }

    std::optional<DrawableId> add_line_drawable(std::span<const float> vertices,
                                                std::span<const std::uint32_t> indices,
                                                std::span<const float> colors,
                                                opengl::LineType lineType,
                                                float lineWidth,
                                                float pointSize,
                                                opengl::BufferAccessPattern accessPattern,
                                                bool dashEnabled = false,
                                                float dashSize = 10.0F,
                                                float gapSize = 10.0F,
                                                renderer::DashSpace dashSpace = renderer::DashSpace::World,
                                                std::span<const std::uint8_t> perVertexDashFlags = {}) {
        auto drawable = opengl::make_line_drawable(get_line_program(),
                                                   vertices,
                                                   3,
                                                   indices,
                                                   colors,
                                                   4,
                                                   lineType,
                                                   lineWidth,
                                                   pointSize,
                                                   accessPattern,
                                                   dashEnabled,
                                                   dashSize,
                                                   gapSize,
                                                   dashSpace,
                                                   perVertexDashFlags);
        if (!drawable.has_value()) {
            return std::nullopt;
        }

        return add_line_drawable(std::move(drawable.value()));
    }

    std::optional<DrawableId> add_mesh_drawable(std::span<const float> vertices,
                                                std::int32_t vertexDimension,
                                                std::span<const float> normals,
                                                std::span<const float> colors,
                                                std::int32_t colorDimension,
                                                std::span<const std::uint32_t> triangleIndices,
                                                opengl::BufferAccessPattern accessPattern) {
        std::vector<float> textureCoordinates((vertices.size() / static_cast<std::size_t>(vertexDimension)) * 2U, 0.0F);
        auto drawable = opengl::make_mesh_soup(get_mesh_program(),
                                               vertices,
                                               vertexDimension,
                                               normals,
                                               textureCoordinates,
                                               colors,
                                               colorDimension,
                                               triangleIndices,
                                               accessPattern);
        if (!drawable.has_value()) {
            return std::nullopt;
        }

        const DrawableId id = next_drawable_id();
        m_meshDrawables.emplace_back(DrawableEntry<opengl::MeshDrawable>{id, std::move(drawable.value())});
        return id;
    }

    std::optional<DrawableId>
    create_texture_2d(const renderer::TextureData& data, int maxTextureSize, int maxAnisotropy) {
        auto texture = Texture2D::create(data, maxTextureSize, maxAnisotropy);
        if (!texture)
            return std::nullopt;
        const DrawableId id = m_nextTextureId++;
        m_textures.emplace(id, std::make_shared<Texture2D>(std::move(*texture)));
        return id;
    }

    std::shared_ptr<Texture2D> get_texture(DrawableId id) const {
        const auto it = m_textures.find(id);
        return it == m_textures.end() ? nullptr : it->second;
    }

    bool remove_texture(DrawableId id) {
        const auto it = m_textures.find(id);
        if (it == m_textures.end() || it->second.use_count() > 1)
            return false;
        m_textures.erase(it);
        return true;
    }

    std::optional<DrawableId> add_textured_mesh_drawable(std::span<const float> vertices,
                                                         std::int32_t vertexDimension,
                                                         std::span<const float> normals,
                                                         std::span<const float> textureCoordinates,
                                                         std::span<const float> colors,
                                                         std::int32_t colorDimension,
                                                         std::span<const std::uint32_t> triangleIndices,
                                                         DrawableId textureId,
                                                         opengl::BufferAccessPattern accessPattern) {
        const auto texture = get_texture(textureId);
        if (!texture)
            return std::nullopt;
        auto drawable = opengl::make_mesh_soup(get_mesh_program(),
                                               vertices,
                                               vertexDimension,
                                               normals,
                                               textureCoordinates,
                                               colors,
                                               colorDimension,
                                               triangleIndices,
                                               accessPattern,
                                               texture);
        if (!drawable)
            return std::nullopt;
        const DrawableId id = next_drawable_id();
        m_meshDrawables.emplace_back(DrawableEntry<opengl::MeshDrawable>{id, std::move(*drawable)});
        return id;
    }

    bool remove_point_drawable(DrawableId id) { return remove_drawable_by_id(m_pointDrawables, id); }

    bool remove_line_drawable(DrawableId id) { return remove_drawable_by_id(m_lineDrawables, id); }

    bool remove_mesh_drawable(DrawableId id) {
        m_meshCullModes.erase(id);
        return remove_drawable_by_id(m_meshDrawables, id);
    }

    void set_mesh_drawable_cull_mode(DrawableId id, MeshCullFaceMode mode) {
        if (mode == MeshCullFaceMode::BACK)
            m_meshCullModes.erase(id); // back is the default; no need to store
        else
            m_meshCullModes[id] = mode;
    }

    void remove_mesh_drawable_cull_mode(DrawableId id) { m_meshCullModes.erase(id); }

    bool set_point_drawable_transform(DrawableId id, const linal::hmatf& transform) {
        return set_drawable_transform_by_id(m_pointDrawables, id, transform);
    }
    [[nodiscard]]
    std::optional<linal::hmatf> get_point_drawable_transform(DrawableId id) const {
        return get_drawable_transform_by_id(m_pointDrawables, id);
    }

    bool set_line_drawable_transform(DrawableId id, const linal::hmatf& transform) {
        return set_drawable_transform_by_id(m_lineDrawables, id, transform);
    }
    [[nodiscard]]
    std::optional<linal::hmatf> get_line_drawable_transform(DrawableId id) const {
        return get_drawable_transform_by_id(m_lineDrawables, id);
    }

    bool set_line_dash_enabled(DrawableId id, bool enabled) {
        return mutate_line_drawable_by_id(id, [enabled](opengl::LineDrawable& d) { d.set_line_dash_enabled(enabled); });
    }
    bool set_line_dash(DrawableId id, float dashSize, float gapSize) {
        return mutate_line_drawable_by_id(
            id, [dashSize, gapSize](opengl::LineDrawable& d) { d.set_line_dash(dashSize, gapSize); });
    }
    bool set_line_dash_phase(DrawableId id, float phase) {
        return mutate_line_drawable_by_id(id, [phase](opengl::LineDrawable& d) { d.set_line_dash_phase(phase); });
    }
    bool set_line_dash_space(DrawableId id, renderer::DashSpace space) {
        return mutate_line_drawable_by_id(id, [space](opengl::LineDrawable& d) { d.set_line_dash_space(space); });
    }

    bool set_mesh_drawable_transform(DrawableId id, const linal::hmatf& transform) {
        return set_drawable_transform_by_id(m_meshDrawables, id, transform);
    }
    [[nodiscard]]
    std::optional<linal::hmatf> get_mesh_drawable_transform(DrawableId id) const {
        return get_drawable_transform_by_id(m_meshDrawables, id);
    }

    bool update_last_point_drawable(std::span<const float> vertices,
                                    std::span<const float> colors,
                                    std::span<const std::uint32_t> indices,
                                    opengl::BufferAccessPattern accessPattern) {
        if (m_pointDrawables.empty()) {
            return false;
        }
        m_pointDrawables.back().drawable.update_vertex_buffer(vertices, accessPattern);
        m_pointDrawables.back().drawable.update_color_buffer(colors, accessPattern);
        m_pointDrawables.back().drawable.update_indices_buffer(indices, accessPattern);
        return true;
    }

    bool update_last_line_drawable(std::span<const float> vertices,
                                   std::span<const float> colors,
                                   std::span<const std::uint32_t> indices,
                                   opengl::BufferAccessPattern accessPattern) {
        if (m_lineDrawables.empty()) {
            return false;
        }
        m_lineDrawables.back().drawable.update_vertex_buffer(vertices, accessPattern);
        m_lineDrawables.back().drawable.update_color_buffer(colors, accessPattern);
        m_lineDrawables.back().drawable.update_indices_buffer(indices, accessPattern);
        return true;
    }

    bool clear_point_drawables() {
        const bool changed = !m_pointDrawables.empty();
        m_pointDrawables.clear();
        return changed;
    }

    bool clear_line_drawables() {
        const bool changed = !m_lineDrawables.empty();
        m_lineDrawables.clear();
        return changed;
    }

    bool clear_mesh_drawables() {
        const bool changed = !m_meshDrawables.empty();
        m_meshDrawables.clear();
        m_meshCullModes.clear();
        return changed;
    }

    bool clear_drawables() {
        const bool pointsChanged = clear_point_drawables();
        const bool linesChanged = clear_line_drawables();
        const bool meshesChanged = clear_mesh_drawables();
        return pointsChanged || linesChanged || meshesChanged;
    }

    void draw_points(const linal::hmatf& mvp) const {
        for (const auto& entry: m_pointDrawables) {
            entry.drawable.draw(mvp, entry.transform);
        }
    }

    void draw_lines(const linal::hmatf& mvp, const linal::float2& viewportSize) const {
        for (const auto& entry: m_lineDrawables) {
            entry.drawable.draw(mvp, entry.transform, viewportSize);
        }
    }

    void draw_lines_and_points(const linal::hmatf& mvp,
                               const linal::float2& viewportSize,
                               const linal::double3& viewPosition) {
        struct RenderCommand {
            enum class Type {
                line,
                point
            };

            Type type{};
            std::size_t index{};
        };

        struct TransparentRenderCommand {
            RenderCommand::Type type{};
            std::size_t index{};
            double distanceSquared{};
        };

        struct RenderQueue {
            std::vector<RenderCommand> opaqueCommands;
            std::vector<TransparentRenderCommand> transparentCommands;
        };

        RenderQueue renderQueue;
        renderQueue.opaqueCommands.reserve(m_lineDrawables.size() + m_pointDrawables.size());
        renderQueue.transparentCommands.reserve(m_lineDrawables.size() + m_pointDrawables.size());

        for (std::size_t i = 0; i < m_lineDrawables.size(); ++i) {
            const auto& drawable = m_lineDrawables[i].drawable;
            if (drawable.has_opaque_primitives()) {
                renderQueue.opaqueCommands.push_back({RenderCommand::Type::line, i});
            }
            if (drawable.has_translucent_primitives()) {
                renderQueue.transparentCommands.push_back(
                    {RenderCommand::Type::line,
                     i,
                     drawable.distance_squared_to(viewPosition, m_lineDrawables[i].transform)});
            }
        }

        for (std::size_t i = 0; i < m_pointDrawables.size(); ++i) {
            const auto& drawable = m_pointDrawables[i].drawable;
            if (drawable.has_opaque_primitives()) {
                renderQueue.opaqueCommands.push_back({RenderCommand::Type::point, i});
            }
            if (drawable.has_translucent_primitives()) {
                renderQueue.transparentCommands.push_back(
                    {RenderCommand::Type::point,
                     i,
                     drawable.distance_squared_to(viewPosition, m_pointDrawables[i].transform)});
            }
        }

        for (const auto& opaqueCommand: renderQueue.opaqueCommands) {
            if (opaqueCommand.type == RenderCommand::Type::line) {
                m_lineDrawables[opaqueCommand.index].drawable.draw_opaque(
                    mvp,
                    m_lineDrawables[opaqueCommand.index].transform,
                    viewportSize);
            } else {
                m_pointDrawables[opaqueCommand.index].drawable.draw_opaque(
                    mvp,
                    m_pointDrawables[opaqueCommand.index].transform);
            }
        }

        std::sort(renderQueue.transparentCommands.begin(),
                  renderQueue.transparentCommands.end(),
                  [](const TransparentRenderCommand& lhs, const TransparentRenderCommand& rhs) {
                      return lhs.distanceSquared > rhs.distanceSquared;
                  });

        if (renderQueue.transparentCommands.empty()) {
            return;
        }

        const ScopedDepthMask depthMask(GL_FALSE);
        for (const auto& transparentCommand: renderQueue.transparentCommands) {
            if (transparentCommand.type == RenderCommand::Type::line) {
                m_lineDrawables[transparentCommand.index].drawable.draw_translucent(
                    mvp,
                    m_lineDrawables[transparentCommand.index].transform,
                    viewportSize,
                    viewPosition);
            } else {
                m_pointDrawables[transparentCommand.index].drawable.draw_translucent(
                    mvp,
                    m_pointDrawables[transparentCommand.index].transform,
                    viewPosition);
            }
        }
    }

    void draw_meshes(const linal::hmatf& viewMatrix,
                     const linal::hmatf& projectionMatrix,
                     const linal::float3& viewPos,
                     const LightingConfig& lighting = LightingConfig{}) const {
        struct TransparentMesh {
            std::size_t index{};
            double distanceSquared{};
        };

        const linal::double3 viewPositionDouble{static_cast<double>(viewPos[0]),
                                                static_cast<double>(viewPos[1]),
                                                static_cast<double>(viewPos[2])};
        std::vector<TransparentMesh> transparentMeshes;
        transparentMeshes.reserve(m_meshDrawables.size());

        for (std::size_t i = 0; i < m_meshDrawables.size(); ++i) {
            const DrawableEntry<opengl::MeshDrawable>& entry = m_meshDrawables[i];
            const auto& drawable = entry.drawable;

            // Apply per-mesh cull mode before drawing.
            auto cullIt = m_meshCullModes.find(entry.id);
            if (cullIt != m_meshCullModes.end()) {
                switch (cullIt->second) {
                case MeshCullFaceMode::FRONT:
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_FRONT);
                    break;
                case MeshCullFaceMode::NONE: glDisable(GL_CULL_FACE); break;
                case MeshCullFaceMode::BACK:
                default:
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                    break;
                }
            }

            if (drawable.is_translucent()) {
                transparentMeshes.push_back({i, drawable.distance_squared_to(viewPositionDouble, entry.transform)});
            } else {
                const linal::hmatf normalMatrix = linal::hmatf::inverse(entry.transform).transpose();
                drawable.draw(entry.transform,
                              viewMatrix,
                              projectionMatrix,
                              normalMatrix,
                              lighting.lightPosition,
                              viewPos,
                              lighting.lightColor,
                              lighting.fillLightDir,
                              lighting.fillLightColor,
                              lighting.ambientColor,
                              lighting.shininess,
                              lighting.lightAttenuation,
                              lighting.materialAmbient,
                              lighting.materialDiffuse,
                              lighting.materialSpecular);
            }

            // Restore default cull state after a per-mesh override.
            if (cullIt != m_meshCullModes.end()) {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
            }
        }

        std::sort(transparentMeshes.begin(),
                  transparentMeshes.end(),
                  [](const TransparentMesh& lhs, const TransparentMesh& rhs) {
                      return lhs.distanceSquared > rhs.distanceSquared;
                  });

        if (transparentMeshes.empty()) {
            return;
        }

        const ScopedDepthMask depthMask(GL_FALSE);
        const ScopedCullFaceDisabled cullFace;
        for (const auto& transparentMesh: transparentMeshes) {
            const DrawableEntry<opengl::MeshDrawable>& entry = m_meshDrawables[transparentMesh.index];
            const linal::hmatf normalMatrix = linal::hmatf::inverse(entry.transform).transpose();
            entry.drawable.draw(entry.transform,
                                viewMatrix,
                                projectionMatrix,
                                normalMatrix,
                                lighting.lightPosition,
                                viewPos,
                                lighting.lightColor,
                                lighting.fillLightDir,
                                lighting.fillLightColor,
                                lighting.ambientColor,
                                lighting.shininess,
                                lighting.lightAttenuation,
                                lighting.materialAmbient,
                                lighting.materialDiffuse,
                                lighting.materialSpecular);
        }
    }

    // Renders every drawable into the currently-bound framebuffer using a flat color that encodes a
    // per-pass sequential index (see PickId.hpp). Returns the index -> drawable mapping so a pixel
    // read back from the framebuffer can be resolved to a drawable. Depth testing (which the caller
    // must enable) resolves occlusion. The caller owns framebuffer binding, viewport, clear, and
    // depth/blend state.
    [[nodiscard]]
    std::vector<PickEntry> draw_pick_pass(const linal::hmatf& mvp,
                                          const linal::hmatf& viewMatrix,
                                          const linal::hmatf& projectionMatrix,
                                          const linal::float2& viewportSize) const {
        std::vector<PickEntry> entries;
        entries.reserve(m_pointDrawables.size() + m_lineDrawables.size() + m_meshDrawables.size());

        const auto next_color = [&entries](PickDrawableKind kind, DrawableId id) {
            entries.push_back({kind, id});
            // Pass index 0 is the reserved "no hit" clear color, so the first drawable is index 1.
            return encode_pick_index(static_cast<std::uint32_t>(entries.size()));
        };

        for (const auto& entry: m_meshDrawables) {
            const std::array<float, 3> color = next_color(PickDrawableKind::mesh, entry.id);
            entry.drawable.draw_pick(entry.transform, viewMatrix, projectionMatrix, color);
        }
        for (const auto& entry: m_lineDrawables) {
            const std::array<float, 3> color = next_color(PickDrawableKind::line, entry.id);
            entry.drawable.draw_pick(mvp, entry.transform, viewportSize, color);
        }
        for (const auto& entry: m_pointDrawables) {
            const std::array<float, 3> color = next_color(PickDrawableKind::point, entry.id);
            entry.drawable.draw_pick(mvp, entry.transform, color);
        }

        return entries;
    }

  private:
    DrawablesManager(opengl::ProgramManager manager)
        : programManager(std::move(manager)) {}

    LineProgram& get_line_program() { return programManager.get_line_program(); }
    PointProgram& get_point_program() { return programManager.get_point_program(); }
    MeshProgram& get_mesh_program() { return programManager.get_mesh_program(); }

    DrawableId next_drawable_id() { return m_nextDrawableId++; }

    template <typename Fn>
    bool mutate_line_drawable_by_id(DrawableId id, Fn&& fn) {
        if (id == 0U) {
            return false;
        }
        const auto it = std::find_if(m_lineDrawables.begin(),
                                     m_lineDrawables.end(),
                                     [id](const DrawableEntry<opengl::LineDrawable>& entry) { return entry.id == id; });
        if (it == m_lineDrawables.end()) {
            return false;
        }
        fn(it->drawable);
        return true;
    }

    template <typename Drawable>
    static bool remove_drawable_by_id(std::vector<DrawableEntry<Drawable>>& drawables, DrawableId id) {
        if (id == 0U) {
            return false;
        }

        const auto it = std::find_if(drawables.begin(), drawables.end(), [id](const DrawableEntry<Drawable>& entry) {
            return entry.id == id;
        });
        if (it == drawables.end()) {
            return false;
        }

        drawables.erase(it);
        return true;
    }

    template <typename Drawable>
    static bool set_drawable_transform_by_id(std::vector<DrawableEntry<Drawable>>& drawables,
                                             DrawableId id,
                                             const linal::hmatf& transform) {
        if (id == 0U) {
            return false;
        }

        const auto it = std::find_if(drawables.begin(), drawables.end(), [id](const DrawableEntry<Drawable>& entry) {
            return entry.id == id;
        });
        if (it == drawables.end()) {
            return false;
        }

        it->transform = transform;
        return true;
    }

    template <typename Drawable>
    static std::optional<linal::hmatf>
    get_drawable_transform_by_id(const std::vector<DrawableEntry<Drawable>>& drawables, DrawableId id) {
        if (id == 0U) {
            return std::nullopt;
        }

        const auto it = std::find_if(drawables.begin(), drawables.end(), [id](const DrawableEntry<Drawable>& entry) {
            return entry.id == id;
        });
        if (it == drawables.end()) {
            return std::nullopt;
        }

        return it->transform;
    }
};

} // namespace opengl

#endif // OPENGL_DRAWABLE_DRAWABLESMANAGER_HPP
