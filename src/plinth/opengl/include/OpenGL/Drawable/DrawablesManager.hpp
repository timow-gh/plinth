#ifndef OPENGL_DRAWABLE_DRAWABLESMANAGER_HPP
#define OPENGL_DRAWABLE_DRAWABLESMANAGER_HPP

#include <OpenGL/Drawable/LineDrawable.hpp>
#include <OpenGL/Drawable/MeshDrawable.hpp>
#include <OpenGL/Drawable/PointDrawable.hpp>
#include <OpenGL/OpenGL.hpp>
#include <OpenGL/Programs/ProgramManager.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <linal/hmat.hpp>
#include <linal/vec.hpp>
#include <memory>
#include <numeric>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace opengl {

// Mirrors the renderer-layer mesh cull mode.
enum class MeshCullFaceMode {
    back,  // GL_BACK  (default)
    front, // GL_FRONT
    none,  // cull face disabled
};

struct LightingConfig {
    linal::float3 lightPosition{0.0F, 0.0F, 10.0F};
    linal::float3 lightColor{1.0F, 1.0F, 1.0F};
    linal::float3 fillLightDir{-0.45F, 0.60F, 0.35F};
    linal::float3 fillLightColor{0.2F, 0.2F, 0.3F};
    linal::float3 ambientColor{0.1F, 0.1F, 0.1F};
    float shininess{8.0F};
};

class DrawablesManager {
  public:
    using DrawableId = std::uint64_t;

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

    std::vector<DrawableEntry<opengl::PointDrawable>> m_pointDrawables;
    std::vector<DrawableEntry<opengl::LineDrawable>> m_lineDrawables;
    std::vector<DrawableEntry<opengl::MeshDrawable>> m_meshDrawables;

    std::unordered_map<DrawableId, MeshCullFaceMode> m_meshCullModes;

    std::vector<DrawableEntry<opengl::LineDrawable>> m_meshSegmentDrawables;

    std::vector<DrawableEntry<opengl::PointDrawable>> m_meshVertexDrawables;

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
        return !m_pointDrawables.empty() || !m_lineDrawables.empty() || !m_meshDrawables.empty() ||
               !m_meshSegmentDrawables.empty() || !m_meshVertexDrawables.empty();
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
    [[nodiscard]]
    bool has_mesh_segment_drawables() const {
        return !m_meshSegmentDrawables.empty();
    }
    [[nodiscard]]
    bool has_mesh_vertex_drawables() const {
        return !m_meshVertexDrawables.empty();
    }

    // Collects a position buffer (world-space xyz triplets, transformed by each drawable's
    // current transform) for every currently-added drawable, for use with
    // renderer::calculate_camera_auto_fit. Unlike the untransformed local vertex data a drawable
    // caches internally, this must return owned data - applying a transform produces new values,
    // not a view into existing memory.
    [[nodiscard]]
    std::vector<std::vector<float>> collect_vertex_position_buffers() const {
        std::vector<std::vector<float>> buffers;
        buffers.reserve(m_pointDrawables.size() + m_lineDrawables.size() + m_meshDrawables.size() +
                       m_meshSegmentDrawables.size() + m_meshVertexDrawables.size());
        const auto collect = [&buffers](const auto& drawables) {
            for (const auto& entry: drawables) {
                const auto span = entry.drawable.get_vertex_positions();
                if (span.empty()) {
                    continue;
                }
                std::vector<float> transformed;
                transformed.reserve(span.size());
                for (std::size_t i = 0; i + 2 < span.size(); i += 3) {
                    const linal::float3 transformedPoint =
                        linal::to_vec(entry.transform * linal::to_hvec(linal::float3{span[i], span[i + 1], span[i + 2]}));
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
        collect(m_meshSegmentDrawables);
        collect(m_meshVertexDrawables);
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
                                                opengl::BufferAccessPattern accessPattern) {
        auto drawable = opengl::make_line_drawable(get_line_program(),
                                                   vertices,
                                                   3,
                                                   indices,
                                                   colors,
                                                   4,
                                                   lineType,
                                                   lineWidth,
                                                   pointSize,
                                                   accessPattern);
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
        auto drawable = opengl::make_mesh_soup(get_mesh_program(),
                                               vertices,
                                               vertexDimension,
                                               normals,
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

    bool remove_point_drawable(DrawableId id) { return remove_drawable_by_id(m_pointDrawables, id); }

    bool remove_line_drawable(DrawableId id) { return remove_drawable_by_id(m_lineDrawables, id); }

    bool remove_mesh_drawable(DrawableId id) {
        m_meshCullModes.erase(id);
        return remove_drawable_by_id(m_meshDrawables, id);
    }

    void set_mesh_drawable_cull_mode(DrawableId id, MeshCullFaceMode mode) {
        if (mode == MeshCullFaceMode::back)
            m_meshCullModes.erase(id); // back is the default; no need to store
        else
            m_meshCullModes[id] = mode;
    }

    void remove_mesh_drawable_cull_mode(DrawableId id) { m_meshCullModes.erase(id); }

    std::optional<DrawableId> add_mesh_segment_drawable(std::span<const float> positions,
                                                        std::span<const std::uint32_t> indices,
                                                        std::span<const float> color,
                                                        float lineWidth) {
        // Expand single RGBA color to per-vertex colors.
        const std::size_t vertexCount = positions.size() / 3;
        std::vector<float> expandedColors;
        if (color.size() == 4 && vertexCount > 0) {
            expandedColors.resize(vertexCount * 4);
            for (std::size_t v = 0; v < vertexCount; ++v)
                for (std::size_t c = 0; c < 4; ++c)
                    expandedColors[v * 4 + c] = color[c];
        } else {
            expandedColors.assign(color.begin(), color.end());
        }

        auto drawable = opengl::make_line_drawable(get_line_program(),
                                                   positions,
                                                   3,
                                                   indices,
                                                   std::span<const float>(expandedColors),
                                                   4,
                                                   opengl::LineType::lines(),
                                                   lineWidth,
                                                   1.0f,
                                                   opengl::BufferAccessPattern::STATIC_DRAW);
        if (!drawable.has_value()) {
            return std::nullopt;
        }

        const DrawableId id = next_drawable_id();
        m_meshSegmentDrawables.emplace_back(DrawableEntry<opengl::LineDrawable>{id, std::move(drawable.value())});
        return id;
    }

    bool remove_mesh_segment_drawable(DrawableId id) { return remove_drawable_by_id(m_meshSegmentDrawables, id); }

    std::optional<DrawableId>
    add_mesh_vertex_drawable(std::span<const float> positions, std::span<const float> color, float pointSize) {
        const std::size_t vertexCount = positions.size() / 3;

        // Expand single RGBA color to per-vertex colors.
        std::vector<float> expandedColors;
        if (color.size() == 4 && vertexCount > 0) {
            expandedColors.resize(vertexCount * 4);
            for (std::size_t v = 0; v < vertexCount; ++v)
                for (std::size_t c = 0; c < 4; ++c)
                    expandedColors[v * 4 + c] = color[c];
        } else {
            expandedColors.assign(color.begin(), color.end());
        }

        std::vector<std::uint32_t> indices(vertexCount);
        std::iota(indices.begin(), indices.end(), 0u);

        auto drawable = opengl::make_point_drawable(get_point_program(),
                                                    positions,
                                                    3,
                                                    std::span<const float>(expandedColors),
                                                    4,
                                                    std::span<const std::uint32_t>(indices),
                                                    pointSize,
                                                    opengl::BufferAccessPattern::STATIC_DRAW);
        if (!drawable.has_value()) {
            return std::nullopt;
        }

        const DrawableId id = next_drawable_id();
        m_meshVertexDrawables.emplace_back(DrawableEntry<opengl::PointDrawable>{id, std::move(drawable.value())});
        return id;
    }

    bool remove_mesh_vertex_drawable(DrawableId id) { return remove_drawable_by_id(m_meshVertexDrawables, id); }

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

    bool set_mesh_drawable_transform(DrawableId id, const linal::hmatf& transform) {
        return set_drawable_transform_by_id(m_meshDrawables, id, transform);
    }
    [[nodiscard]]
    std::optional<linal::hmatf> get_mesh_drawable_transform(DrawableId id) const {
        return get_drawable_transform_by_id(m_meshDrawables, id);
    }

    bool set_mesh_segment_drawable_transform(DrawableId id, const linal::hmatf& transform) {
        return set_drawable_transform_by_id(m_meshSegmentDrawables, id, transform);
    }
    [[nodiscard]]
    std::optional<linal::hmatf> get_mesh_segment_drawable_transform(DrawableId id) const {
        return get_drawable_transform_by_id(m_meshSegmentDrawables, id);
    }

    bool set_mesh_vertex_drawable_transform(DrawableId id, const linal::hmatf& transform) {
        return set_drawable_transform_by_id(m_meshVertexDrawables, id, transform);
    }
    [[nodiscard]]
    std::optional<linal::hmatf> get_mesh_vertex_drawable_transform(DrawableId id) const {
        return get_drawable_transform_by_id(m_meshVertexDrawables, id);
    }

    // Draw overlay calls ΓÇö draw opaque only, no transparency sorting needed for overlays.
    void draw_mesh_segment_overlays(const linal::hmatf& mvp) const {
        for (const auto& entry: m_meshSegmentDrawables) {
            entry.drawable.draw_opaque(mvp, entry.transform);
        }
    }

    void draw_mesh_vertex_overlays(const linal::hmatf& mvp) const {
        for (const auto& entry: m_meshVertexDrawables) {
            entry.drawable.draw_opaque(mvp, entry.transform);
        }
    }

    void update_last_point_drawable(std::span<const float> vertices,
                                    std::span<const float> colors,
                                    std::span<const std::uint32_t> indices,
                                    opengl::BufferAccessPattern accessPattern) {
        if (m_pointDrawables.empty()) {
            return;
        }
        m_pointDrawables.back().drawable.update_vertex_buffer(vertices, accessPattern);
        m_pointDrawables.back().drawable.update_color_buffer(colors, accessPattern);
        m_pointDrawables.back().drawable.update_indices_buffer(indices, accessPattern);
    }

    void update_last_line_drawable(std::span<const float> vertices,
                                   std::span<const float> colors,
                                   std::span<const std::uint32_t> indices,
                                   opengl::BufferAccessPattern accessPattern) {
        if (m_lineDrawables.empty()) {
            return;
        }
        m_lineDrawables.back().drawable.update_vertex_buffer(vertices, accessPattern);
        m_lineDrawables.back().drawable.update_color_buffer(colors, accessPattern);
        m_lineDrawables.back().drawable.update_indices_buffer(indices, accessPattern);
    }

    void clear_point_drawables() { m_pointDrawables.clear(); }

    void clear_line_drawables() { m_lineDrawables.clear(); }

    void clear_mesh_drawables() {
        m_meshDrawables.clear();
        m_meshCullModes.clear();
    }

    void clear_mesh_segment_drawables() { m_meshSegmentDrawables.clear(); }
    void clear_mesh_vertex_drawables() { m_meshVertexDrawables.clear(); }

    void clear_drawables() {
        clear_point_drawables();
        clear_line_drawables();
        clear_mesh_drawables();
        clear_mesh_segment_drawables();
        clear_mesh_vertex_drawables();
    }

    void draw_points(const linal::hmatf& mvp) const {
        for (const auto& entry: m_pointDrawables) {
            entry.drawable.draw(mvp, entry.transform);
        }
    }

    void draw_lines(const linal::hmatf& mvp) const {
        for (const auto& entry: m_lineDrawables) {
            entry.drawable.draw(mvp, entry.transform);
        }
    }

    void draw_lines_and_points(const linal::hmatf& mvp, const linal::double3& viewPosition) {
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
                    mvp, m_lineDrawables[opaqueCommand.index].transform);
            } else {
                m_pointDrawables[opaqueCommand.index].drawable.draw_opaque(
                    mvp, m_pointDrawables[opaqueCommand.index].transform);
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
                    mvp, m_lineDrawables[transparentCommand.index].transform, viewPosition);
            } else {
                m_pointDrawables[transparentCommand.index].drawable.draw_translucent(
                    mvp, m_pointDrawables[transparentCommand.index].transform, viewPosition);
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

        // Enable polygon offset fill when segment overlays are present
        // so the surface sits slightly behind the wireframe lines.
        const bool hasSegmentOverlays = !m_meshSegmentDrawables.empty();
        if (hasSegmentOverlays) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.0f);
        }

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
                case MeshCullFaceMode::front:
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_FRONT);
                    break;
                case MeshCullFaceMode::none: glDisable(GL_CULL_FACE); break;
                case MeshCullFaceMode::back:
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
                              lighting.shininess);
            }

            // Restore default cull state after a per-mesh override.
            if (cullIt != m_meshCullModes.end()) {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
            }
        }

        if (hasSegmentOverlays) {
            glDisable(GL_POLYGON_OFFSET_FILL);
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
                                lighting.shininess);
        }
    }

  private:
    DrawablesManager(opengl::ProgramManager manager)
        : programManager(std::move(manager)) {}

    LineProgram& get_line_program() { return programManager.get_line_program(); }
    PointProgram& get_point_program() { return programManager.get_point_program(); }
    MeshProgram& get_mesh_program() { return programManager.get_mesh_program(); }

    DrawableId next_drawable_id() { return m_nextDrawableId++; }

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
    static bool
    set_drawable_transform_by_id(std::vector<DrawableEntry<Drawable>>& drawables, DrawableId id, const linal::hmatf& transform) {
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
    static std::optional<linal::hmatf> get_drawable_transform_by_id(const std::vector<DrawableEntry<Drawable>>& drawables,
                                                                    DrawableId id) {
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
