# Sphere-point rendering

This note describes why sphere points are implemented as impostors, how data moves through the
renderer, and which invariants must remain true when the feature is extended. The public entry
point is `Renderer::add_sphere_point_drawable`.

## Why impostors

A sphere point is conceptually a local-space center and radius, not a triangle mesh. Rendering a
tessellated sphere for every point would multiply vertex data and draw work as sphere count or
visual quality grows. Instead, plinth stores eight floats per instance and emits a two-triangle
screen-space proxy from `gl_VertexID`. The fragment shader ray-traces the actual surface inside
that proxy.

This split gives sphere points:

- constant geometry cost per instance;
- smooth silhouettes independent of tessellation;
- accurate surface depth for normal rendering and picking; and
- ordinary model transforms, including non-uniform scale and shear.

The proxy is only a conservative rasterization region. It is deliberately allowed to be larger
than the visible surface; the fragment shader discards rays that miss. Tightening the proxy is a
performance optimization and must never make it smaller than the transformed sphere silhouette.

## Ownership and data flow

The feature crosses several layers intentionally:

1. `Renderer` owns the public operation, expands a uniform color, requests auto-fit, and returns a
   typed `DrawableHandle`; lower layers reject inconsistent or empty input arrays.
2. `DrawablesManager` owns sphere drawables, schedules opaque/translucent rendering, and assigns
   one picking ID per drawable.
3. `SphereImpostorDrawable` owns the VAO and separate opaque/translucent instance buffers. It
   transforms CPU-side lighting inputs, uploads uniforms, sorts translucent instances, and issues
   instanced draws.
4. `SphereImpostorProgram` is the C++ side of the shader interface. It keeps uniform and attribute
   locations alive and asserts that the expected interface was linked.
5. `ShaderSources.cpp` constructs the proxy in the vertex shader and performs intersection,
   depth, picking, and lighting in the fragment shader.

The instance layout is shared by `SphereInstanceData.hpp`, `SortableSphereInstance`, the buffer
builder, and the shader attributes:

```text
center.x, center.y, center.z, radius, red, green, blue, alpha
```

Keeping this compact interleaved layout makes one instanced buffer sufficient for each opacity
class. Translucent instances also retain a CPU copy because their order can change with the camera.

## Coordinate-space contract

Coordinate spaces are the most important invariant in this implementation:

- Instance centers and radii are local to the drawable.
- `u_model` maps local space to world space.
- `u_view` maps world space to view space.
- Camera positions used by CPU sorting are world-space values.
- Point-light positions and fill-light directions enter the renderer in world space, then
  `SphereImpostorDrawable` uploads them in view space.
- Fragment lighting, including the view direction, is entirely in view space.

The vertex shader converts the local center to view space and computes a conservative view-space
radius using an upper bound on the model-view matrix norm. It uses that radius only to size the
proxy. For diagonal scaling the bound is exact; rotations and shears may cause harmless overdraw.

The fragment shader unprojects a view-space ray and transforms the ray into local space with the
full inverse model-view matrix. Intersecting the original local sphere there is what makes affine
model transforms work without inventing separate ellipsoid intersection code. The hit is then
transformed back to view space to calculate `gl_FragDepth`.

Normals use the inverse transpose of model-view. Do not transform them with `mat3(u_model)` or
derive them only from view-space center-to-hit vectors: both approaches break under non-uniform
scale. The matrix inverse is computed with GLM because `linal::hmatf::inverse` is intended for
orthogonal transforms and is not a general affine inverse.

Transforms must be non-singular. A zero scale has no inverse and is outside the drawable contract.

## Opaque, translucent, and picking passes

Opaque and translucent instances are split when the drawable is built. This avoids blending and
sorting work for opaque spheres while allowing a single public drawable to contain both kinds.

Transparency is sorted at two levels:

1. `DrawablesManager` sorts translucent sphere drawables by their transformed aggregate center.
2. `SphereImpostorDrawable` transforms each local instance center through `u_model`, sorts those
   world-space centers against the world-space camera, and streams the reordered instance data.

Both comparisons are back-to-front. Center sorting is the usual alpha-blending approximation; it
cannot perfectly order intersecting or very large transparent surfaces. A future order-independent
transparency implementation should replace both levels together.

Picking uses the same proxy, local-space intersection, model transform, and corrected depth as the
visible pass. Only the final color changes to the encoded picking color. This parity is deliberate:
a simplified billboard pick path would select transparent proxy corners or return the wrong object
where transformed spheres overlap. A picking ID identifies the whole drawable, not an individual
sphere instance. Per-instance selection would require instance identity in the picking map and
shader output, not merely another color attribute.

## Shader interface maintenance

Shader names are a runtime interface, so adding or changing an attribute or uniform requires all
of the following updates in the same change:

1. Declare and use it in the appropriate shader source.
2. Add storage, constructor plumbing, move plumbing, and a getter in `SphereImpostorProgram`.
3. Resolve it in `make_sphere_impostor_program` and pass it to the constructor.
4. Upload it in every applicable path in `SphereImpostorDrawable`, including `draw_pick` when the
   intersection or depth calculation needs it.
5. Add a graphics regression that would fail if the value were absent or in the wrong space.

The program asserts active locations to catch drift early. A declared value that GLSL optimizes
away is not active, so a new uniform must affect shader output or should not be part of the program
contract.

When extending per-instance data, also update `kSphereInstanceFloats`, the stride and attribute
offsets, `SortableSphereInstance::data`, both buffer-building paths, and the shader inputs. Prefer a
named layout type if the instance record grows beyond the current center/radius/color tuple.

## Current limitations

- Scene bounds currently receive sphere centers through the generic vertex-position interface;
  radii are not included. Extending auto-fit correctly requires an extent-aware bounds contract
  rather than disguising radius samples as vertices.
- There is no in-place public update API for sphere points. The picking example demonstrates the
  current remove-and-rebuild approach when recoloring.
- Alpha blending uses center-based sorting and has the overlap limitation described above.
- Singular model transforms are unsupported.

These limitations should be made explicit in any API that changes them so callers do not depend
on accidental behavior.

## Regression coverage

`SphereImpostorRendererTest` covers the behaviors most likely to regress:

- perspective and orthographic projection;
- legacy and zero-to-one/reversed depth;
- visible and picking depth;
- non-uniform transformed geometry;
- camera-relative lighting;
- transformed translucent-instance sorting; and
- mixed opaque/translucent rendering.

When changing proxy bounds, intersection math, coordinate spaces, depth conventions, or sorting,
add a focused image/picking assertion before broadening the implementation.
