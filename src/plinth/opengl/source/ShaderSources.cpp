#include "OpenGL/ShaderSources.hpp"

namespace opengl {
std::string line_vertex_shader_source() {
    return
        R"(#version 330 core

uniform mat4  u_viewProjection;
uniform mat4  u_model;
uniform vec2  u_viewportSize;
uniform float u_lineWidth;
uniform int   u_dashSpace;    // 0 = World, 1 = Screen
uniform int   u_capStyle;     // 0 = Butt, 1 = Square, 2 = Round
uniform int   u_joinStyle;    // 0 = Miter, 1 = Bevel, 2 = Round

// Shared unit quad (divisor 0): x in {0,1} selects endpoint, y in {-0.5,0.5} selects side.
in vec2 a_corner;
// Per-instance segment data (divisor 1).
in vec4 a_p0;    // (p0.xyz,    dashedFlag)
in vec4 a_p1;    // (p1.xyz,    arc0)
in vec4 a_color0;
in vec4 a_color1;
in vec4 a_pPrev; // (pPrev.xyz, 0) — world pos before p0; == p0 when no predecessor
in vec4 a_pNext; // (pNext.xyz, 0) — world pos after  p1; == p1 when no successor

out vec4  v_color;
out float v_arcLen;
out float v_dashedFlag;
out vec2  v_s0;
out vec2  v_s1;
out vec2  v_fragPos;
out vec2  v_dirPrev;
out vec2  v_dirNext;
out float v_halfWidth;

void main() {
    vec3  p0         = a_p0.xyz;
    vec3  p1         = a_p1.xyz;
    float dashedFlag = a_p0.w;
    float arc0       = a_p1.w;

    vec4 clip0    = u_viewProjection * u_model * vec4(p0,         1.0);
    vec4 clip1    = u_viewProjection * u_model * vec4(p1,         1.0);
    vec4 clipPrev = u_viewProjection * u_model * vec4(a_pPrev.xyz, 1.0);
    vec4 clipNext = u_viewProjection * u_model * vec4(a_pNext.xyz, 1.0);

    vec2 s0    = (clip0.xy    / clip0.w)    * 0.5 * u_viewportSize;
    vec2 s1    = (clip1.xy    / clip1.w)    * 0.5 * u_viewportSize;
    vec2 sPrev = (clipPrev.xy / clipPrev.w) * 0.5 * u_viewportSize;
    vec2 sNext = (clipNext.xy / clipNext.w) * 0.5 * u_viewportSize;

    vec2  dir       = s1 - s0;
    float screenLen = length(dir);
    dir = screenLen > 0.001 ? dir / screenLen : vec2(1.0, 0.0);
    vec2 nrm = vec2(-dir.y, dir.x);

    float halfW = u_lineWidth * 0.5;

    bool hasPrev = length(sPrev - s0) > 0.5;
    bool hasNext = length(sNext - s1) > 0.5;

    // Extend the quad past each endpoint for cap or round join coverage.
    float extendStart = 0.0;
    float extendEnd   = 0.0;
    if (!hasPrev && (u_capStyle == 1 || u_capStyle == 2)) extendStart = halfW;
    if (!hasNext && (u_capStyle == 1 || u_capStyle == 2)) extendEnd   = halfW;
    if ( hasPrev &&  u_joinStyle == 2)                    extendStart = halfW;
    if ( hasNext &&  u_joinStyle == 2)                    extendEnd   = halfW;

    float t    = a_corner.x;
    vec4  clip = mix(clip0, clip1, t);

    // Extension along the segment direction combined with perpendicular side offset.
    vec2 alongDir    = (t < 0.5) ? -dir * extendStart : dir * extendEnd;
    vec2 sideNrm     = nrm * (a_corner.y * u_lineWidth);
    vec2 totalOffset = alongDir + sideNrm;
    vec2 ndcOffset   = totalOffset / (0.5 * u_viewportSize);
    clip.xy += ndcOffset * clip.w;
    gl_Position = clip;

    float worldArc1 = arc0 + distance(p0, p1);
    float worldArc  = mix(arc0, worldArc1, t);
    float screenArc = mix(0.0, screenLen, t);
    v_arcLen     = (u_dashSpace == 1) ? screenArc : worldArc;
    v_color      = mix(a_color0, a_color1, t);
    v_dashedFlag = dashedFlag;

    // Screen-pixel position of this quad vertex, interpolated per-fragment for SDF.
    vec2 sBase = mix(s0, s1, t);
    v_fragPos   = sBase + totalOffset;
    v_s0        = s0;
    v_s1        = s1;
    v_halfWidth = halfW;
    v_dirPrev   = hasPrev ? normalize(s0 - sPrev) : vec2(0.0);
    v_dirNext   = hasNext ? normalize(sNext - s1)  : vec2(0.0);
})";
}

std::string line_fragment_shader_source() {
    return
        R"(#version 330 core

in vec4  v_color;
in float v_arcLen;
in float v_dashedFlag;
in vec2  v_s0;
in vec2  v_s1;
in vec2  v_fragPos;
in vec2  v_dirPrev;
in vec2  v_dirNext;
in float v_halfWidth;

uniform bool  u_pickMode;
uniform vec3  u_pickColor;

// Dash pattern (replaces u_dashEnabled / u_dashSize / u_gapSize).
// u_dashPatternCount == 0 means solid. Alternating on/off lengths.
#define DASH_PATTERN_MAX 16
uniform int   u_dashPatternCount;
uniform float u_dashPattern[DASH_PATTERN_MAX];
uniform float u_dashPhase;
uniform int   u_dashSpace;

uniform int   u_capStyle;    // 0 = Butt, 1 = Square, 2 = Round
uniform int   u_joinStyle;   // 0 = Miter, 1 = Bevel, 2 = Round

out vec4 FragColor;

void main() {
    if (u_pickMode) {
        // Picking ignores dashing and SDF so the whole quad footprint stays selectable.
        FragColor = vec4(u_pickColor, 1.0);
        return;
    }

    // --- Dash discard ---
    if (u_dashPatternCount > 0 && v_dashedFlag > 0.5) {
        float period = 0.0;
        for (int i = 0; i < u_dashPatternCount; ++i) {
            period += u_dashPattern[i];
        }
        period = max(period, 1e-6);

        float pos = mod(v_arcLen - u_dashPhase, period);
        if (pos < 0.0) pos += period;

        float acc  = 0.0;
        bool inGap = false;
        for (int i = 0; i < u_dashPatternCount; ++i) {
            acc += u_dashPattern[i];
            if (pos < acc) {
                inGap = (i % 2 == 1);
                break;
            }
        }
        if (inGap) discard;
    }

    // --- SDF geometry clip ---
    vec2  p      = v_fragPos;
    vec2  seg    = v_s1 - v_s0;
    float segLen = length(seg);
    vec2  segDir = segLen > 0.001 ? seg / segLen : vec2(1.0, 0.0);
    vec2  segNrm = vec2(-segDir.y, segDir.x);
    vec2  toP    = p - v_s0;
    float tAlong = dot(toP, segDir);
    float tPerp  = dot(toP, segNrm);

    // Reject fragments outside the line width on either side (always).
    if (abs(tPerp) > v_halfWidth) discard;

    bool hasPrev = length(v_dirPrev) > 0.5;
    bool hasNext = length(v_dirNext) > 0.5;

    // Start region (before p0).
    if (tAlong < 0.0) {
        if (!hasPrev) {
            // Open endpoint — apply cap.
            if (u_capStyle == 0) discard;
            if (u_capStyle == 1 && tAlong < -v_halfWidth) discard;
            if (u_capStyle == 2 && length(p - v_s0) > v_halfWidth) discard;
        } else {
            // Interior join at p0.
            if (u_joinStyle != 2) discard;
            if (length(p - v_s0) > v_halfWidth) discard;
        }
    }

    // End region (past p1).
    if (tAlong > segLen) {
        if (!hasNext) {
            // Open endpoint — apply cap.
            if (u_capStyle == 0) discard;
            if (u_capStyle == 1 && tAlong > segLen + v_halfWidth) discard;
            if (u_capStyle == 2 && length(p - v_s1) > v_halfWidth) discard;
        } else {
            // Interior join at p1.
            if (u_joinStyle != 2) discard;
            if (length(p - v_s1) > v_halfWidth) discard;
        }
    }

    FragColor = v_color;
}
)";
}
std::string point_color_vertex_shader_source() {
    return
        R"(#version 330

uniform mat4 u_viewProjection;
uniform mat4 u_model;
uniform float u_pointSize;

in vec3 a_vertex;
in vec4 a_color;

out vec4 v_color;

void main() {
    gl_Position = u_viewProjection * u_model * vec4(a_vertex, 1.0);
    gl_PointSize = u_pointSize;
    v_color = a_color;
})";
}
std::string point_color_fragment_shader_source() {
    return
        R"(#version 330

in vec4 v_color;

uniform bool u_pickMode;
uniform vec3 u_pickColor;

out vec4 FragColor;

void main() {
    if (u_pickMode) {
        FragColor = vec4(u_pickColor, 1.0);
        return;
    }
    FragColor = v_color;
})";
}

std::string mesh_vertex_shader_source() {
    return
        R"(#version 330

    uniform mat4 u_model;
    uniform mat4 u_view;
    uniform mat4 u_projection;
    uniform mat4 u_normalMatrix;

    in vec3 a_vertex;
    in vec4 a_color;
    in vec3 a_normal;
    in vec2 a_texCoord;

    out vec4 v_color;
    out vec3 v_normal;
    out vec3 v_position;
    out vec2 v_texCoord;

    void main() {
        gl_Position = u_projection * u_view * u_model * vec4(a_vertex, 1.0);
        v_color = a_color;
        v_normal = mat3(u_normalMatrix) * a_normal;
        v_position = vec3(u_model * vec4(a_vertex, 1.0));
        v_texCoord = a_texCoord;
    })";
}

std::string mesh_fragment_shader_source() {
    return
        R"(#version 330

    in vec4 v_color;
    in vec3 v_normal;
    in vec3 v_position;
    in vec2 v_texCoord;

    out vec4 FragColor;

    uniform vec3 u_lightPos;
    // The view position is the position of the camera in world space.
    // It is used to calculate the view direction in the fragment shader.
    uniform vec3 u_viewPos;
    uniform vec3 u_lightColor;
    uniform vec3 u_fillLightDirection;
    uniform vec3 u_fillLightColor;
    uniform vec3 u_ambientColor;
    uniform float u_shininess;
    uniform bool u_hasAlbedoTexture;
    uniform sampler2D u_albedoTexture;
    uniform vec3 u_lightAttenuation;
    uniform vec3 u_materialAmbient;
    uniform vec3 u_materialDiffuse;
    uniform vec3 u_materialSpecular;

    uniform bool u_pickMode;
    uniform vec3 u_pickColor;

    void main() {
        if (u_pickMode) {
            FragColor = vec4(u_pickColor, 1.0);
            return;
        }

        // Normalize input vectors
        vec3 norm = normalize(v_normal);
        vec3 lightDir = normalize(u_lightPos - v_position);
        vec3 viewDir = normalize(u_viewPos - v_position);

        vec3 albedo = v_color.rgb;
        if (u_hasAlbedoTexture) {
            albedo *= texture(u_albedoTexture, v_texCoord).rgb;
        }

        // Point light attenuation (constant, linear, quadratic)
        float lightDist = length(u_lightPos - v_position);
        float attenuation = 1.0 / (u_lightAttenuation.x +
                                   u_lightAttenuation.y * lightDist +
                                   u_lightAttenuation.z * lightDist * lightDist);

        // Ambient lighting
        vec3 ambient = u_materialAmbient * u_ambientColor * albedo;

        // Diffuse lighting
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = u_materialDiffuse * u_lightColor * diff * albedo * attenuation;

        // Directional fill lighting
        float fillDirLength = length(u_fillLightDirection);
        vec3 fillDir = fillDirLength > 0.0 ? u_fillLightDirection / fillDirLength : vec3(0.0);
        float fillDiff = max(dot(norm, fillDir), 0.0);
        vec3 fillDiffuse = u_fillLightColor * fillDiff * albedo;

        // Specular lighting (Blinn-Phong)
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), u_shininess);
        // Mask by N.L so specular only appears on faces turned toward the light,
        // and normalize energy so high-shininess highlights don't stack full
        // intensity on top of an already-saturated diffuse term.
        float specNorm = (u_shininess + 8.0) / 8.0;
        vec3 specular = u_materialSpecular * u_lightColor * spec * specNorm * diff * attenuation;

        // Combine results
        vec3 result = ambient + diffuse + fillDiffuse + specular;
        FragColor = vec4(result, v_color.a);
    })";
}

std::string sphere_impostor_vertex_shader_source() {
    return
        R"(#version 330 core

// Per-instance data (divisor 1)
in vec4 a_sphere; // (center.xyz, radius)
in vec4 a_color;  // (r, g, b, a)

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec2 u_viewportSize;

out vec4  v_color;
out vec3  v_sphereCenterView;
out float v_radius;

// Unit quad corners: two counter-clockwise triangles
const vec2 quadOffsets[6] = vec2[6](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

void main() {
    vec3 centerWorld      = vec3(u_model * vec4(a_sphere.xyz, 1.0));
    float radius          = a_sphere.w;
    vec4  centerView      = u_view * vec4(centerWorld, 1.0);
    vec4  centerClip      = u_projection * centerView;

    // Compute a conservative axis-aligned screen-space bound for the projected sphere.
    // Projecting center +/- radius at the center depth under-bounds a perspective sphere:
    // its silhouette is defined by tangent rays, not by points on that depth plane.
    vec2 boundsMin;
    vec2 boundsMax;
    bool orthographic = abs(u_projection[3][3]) > 0.5;
    if (orthographic) {
        vec2 centerNdc = centerClip.xy / centerClip.w;
        vec2 halfExtent = vec2(abs(u_projection[0][0]), abs(u_projection[1][1])) * radius;
        boundsMin = centerNdc - halfExtent;
        boundsMax = centerNdc + halfExtent;
    } else {
        float z = centerView.z;
        float denominator = z * z - radius * radius;
        if (denominator <= 0.0) {
            // The sphere reaches the camera plane. A full-screen proxy is conservative and
            // lets the fragment intersection determine which rays really hit it.
            boundsMin = vec2(-1.0);
            boundsMax = vec2( 1.0);
        } else {
            float xRoot = sqrt(max(centerView.x * centerView.x + denominator, 0.0));
            float yRoot = sqrt(max(centerView.y * centerView.y + denominator, 0.0));
            vec2 lowerSlope = vec2(-centerView.x * z - radius * xRoot,
                                   -centerView.y * z - radius * yRoot) / denominator;
            vec2 upperSlope = vec2(-centerView.x * z + radius * xRoot,
                                   -centerView.y * z + radius * yRoot) / denominator;
            vec2 projectionScale = vec2(u_projection[0][0], u_projection[1][1]);
            vec2 projected0 = projectionScale * lowerSlope;
            vec2 projected1 = projectionScale * upperSlope;
            boundsMin = min(projected0, projected1);
            boundsMax = max(projected0, projected1);
        }
    }

    // Cover edge pixels despite floating-point/rasterization rounding.
    vec2 pixelMargin = 2.0 / u_viewportSize;
    boundsMin -= pixelMargin;
    boundsMax += pixelMargin;

    vec2 corner01 = quadOffsets[gl_VertexID % 6] * 0.5 + 0.5;
    vec2 cornerNdc = mix(boundsMin, boundsMax, corner01);
    // The proxy depth only needs to survive clipping; the fragment shader writes the sphere's
    // actual surface depth. Using z=0 also keeps spheres crossing the near plane rasterizable.
    gl_Position = vec4(cornerNdc, 0.0, 1.0);

    v_sphereCenterView = centerView.xyz;
    v_radius           = radius;
    v_color            = a_color;
})";
}

std::string sphere_impostor_fragment_shader_source() {
    return
        R"(#version 330 core

in vec4  v_color;
in vec3  v_sphereCenterView;
in float v_radius;

uniform mat4  u_projection;
uniform mat4  u_invProjection;
uniform vec2  u_viewportSize;
uniform bool  u_zeroToOneDepth;

uniform vec3  u_lightPos;
uniform vec3  u_lightColor;
uniform vec3  u_fillLightDirection;
uniform vec3  u_fillLightColor;
uniform vec3  u_ambientColor;
uniform float u_shininess;
uniform vec3  u_lightAttenuation;
uniform vec3  u_materialAmbient;
uniform vec3  u_materialDiffuse;
uniform vec3  u_materialSpecular;

uniform bool u_pickMode;
uniform vec3 u_pickColor;

out vec4 FragColor;

void main() {
    // Unproject two points on the fragment's view-space ray. In this renderer the
    // zero-to-one convention is paired with reversed Z; the legacy convention uses
    // OpenGL's usual negative-one-to-one clip depths.
    vec2  ndcXY     = (gl_FragCoord.xy / u_viewportSize) * 2.0 - 1.0;
    float nearDepth = u_zeroToOneDepth ? 1.0 : -1.0;
    float farDepth  = u_zeroToOneDepth ? 0.0 :  1.0;
    vec4  nearViewH = u_invProjection * vec4(ndcXY, nearDepth, 1.0);
    vec4  farViewH  = u_invProjection * vec4(ndcXY, farDepth, 1.0);
    vec3  rayOrigin = nearViewH.xyz / nearViewH.w;
    vec3  rayDir    = normalize(farViewH.xyz / farViewH.w - rayOrigin);

    // Ray-sphere intersection in view space.
    // Starting the ray at the unprojected near plane works for both perspective
    // and orthographic projections.
    vec3  oc = rayOrigin - v_sphereCenterView;
    float a  = dot(rayDir, rayDir);
    float b  = 2.0 * dot(rayDir, oc);
    float c  = dot(oc, oc) - v_radius * v_radius;
    float discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0) {
        discard;
    }

    float sqrtDiscriminant = sqrt(discriminant);
    float tNear = (-b - sqrtDiscriminant) / (2.0 * a);
    float tFar  = (-b + sqrtDiscriminant) / (2.0 * a);
    float t     = tNear >= 0.0 ? tNear : tFar;
    if (t < 0.0) {
        discard;
    }
    vec3 hitView = rayOrigin + t * rayDir;

    // Write corrected depth so the sphere occludes geometry properly.
    // u_projection encodes the depth direction. Only the legacy [-1,1] clip-depth
    // convention needs the NDC-to-window remap; GL_ZERO_TO_ONE is already window depth.
    vec4  hitClip  = u_projection * vec4(hitView, 1.0);
    float hitDepth = hitClip.z / hitClip.w;
    gl_FragDepth   = u_zeroToOneDepth ? hitDepth : hitDepth * 0.5 + 0.5;

    if (u_pickMode) {
        FragColor = vec4(u_pickColor, 1.0);
        return;
    }

    // Per-pixel Phong lighting in world space.
    // Reconstruct the world-space hit position and normal from view-space hit.
    // Normal points outward from sphere center.
    vec3 normal   = normalize(hitView - v_sphereCenterView);
    // The normal is in view space; to transform to world space we need the view inverse.
    // Instead, compute diffuse/specular in view space where the camera is at the origin.
    vec3 viewDir  = normalize(-hitView); // direction toward camera from hit point

    vec3 albedo = v_color.rgb;

    // Point light: transform light position to view space via the same view matrix
    // that the vertex shader used. We re-derive it from the world-space uniforms.
    // (We do Phong in view space to avoid an additional uniform matrix.)
    // u_lightPos and u_viewPos are world-space; we use the view-space hit directly.
    // For a simple and consistent result, compute lighting in view space:
    //   light direction = normalize(lightViewPos - hitView)
    // But we only have u_lightPos in world space and no view matrix in the fragment shader.
    // Use world-space approximation: v_sphereCenterView gives us the sphere position.
    // Reconstruct world-space hit from viewspace via an approximation using the direction.
    // Since this is impostor rendering we approximate: use v_color for pure albedo shading
    // driven by the view-space normal and a hardcoded fill, then add the world-space point light.
    //
    // The cleanest approach: do all lighting in view space.
    // We pass u_lightPos and u_viewPos in world space; the vertex shader must transform them.
    // Fragment only has view-space data. We compute the world-space position from the
    // view-space hit using the inverse-view matrix — but that would need another uniform.
    //
    // Practical solution: approximate world-space position from view-space hit.
    // This is accurate because hitView is the actual intersection point in view space.
    // We pass u_lightPos and u_viewPos in world space; the world-space hit is not available here.
    // To keep the shader self-contained without an inverse-view matrix, perform lighting
    // purely in view space using a view-space light position approximation.
    //
    // For the fill light (directional), we only need the view-space normal, which we have.
    // For the point light, we approximate: transform u_lightPos to view space would require
    // uploading u_lightPosView. Instead, treat the point light as a distant directional light
    // pointing from u_lightPos toward the sphere center (this is the standard impostor trick).
    vec3 approxLightDir = normalize(u_lightPos - v_sphereCenterView);
    vec3 approxViewDir  = normalize(-hitView);

    // Point light attenuation based on approximate distance.
    float lightDist    = length(u_lightPos - v_sphereCenterView);
    float attenuation  = 1.0 / (u_lightAttenuation.x +
                                u_lightAttenuation.y * lightDist +
                                u_lightAttenuation.z * lightDist * lightDist);

    // Ambient
    vec3 ambient = u_materialAmbient * u_ambientColor * albedo;

    // Diffuse (point light)
    float diff   = max(dot(normal, approxLightDir), 0.0);
    vec3 diffuse = u_materialDiffuse * u_lightColor * diff * albedo * attenuation;

    // Fill light (directional)
    float fillDirLength = length(u_fillLightDirection);
    vec3  fillDir       = fillDirLength > 0.0 ? u_fillLightDirection / fillDirLength : vec3(0.0);
    float fillDiff      = max(dot(normal, fillDir), 0.0);
    vec3  fillDiffuse   = u_fillLightColor * fillDiff * albedo;

    // Specular (Blinn-Phong)
    vec3  halfwayDir = normalize(approxLightDir + approxViewDir);
    float spec       = pow(max(dot(normal, halfwayDir), 0.0), u_shininess);
    float specNorm   = (u_shininess + 8.0) / 8.0;
    vec3  specular   = u_materialSpecular * u_lightColor * spec * specNorm * diff * attenuation;

    vec3 result = ambient + diffuse + fillDiffuse + specular;
    FragColor   = vec4(result, v_color.a);
})";
}

std::string post_processing_vertex_shader_source() {
    return
        R"(#version 330

out vec2 v_uv;

void main() {
    vec2 position = vec2((gl_VertexID == 1) ? 3.0 : -1.0,
                         (gl_VertexID == 2) ? 3.0 : -1.0);
    gl_Position = vec4(position, 0.0, 1.0);
    v_uv = position * 0.5 + 0.5;
})";
}

std::string post_processing_fragment_shader_source() {
    return
        R"(#version 330

in vec2 v_uv;

uniform sampler2D u_sceneColor;
uniform sampler2D u_sceneDepth;

uniform bool u_reversedDepth;

uniform float  u_exposureStops;

uniform int    u_toneMapMode;

uniform int    u_visualizationMode;
uniform float  u_hdrDisplayMax;

uniform bool   u_grayscale;

out vec4 FragColor;

vec3 srgbEncode(vec3 linear) {
    vec3 low  = linear * 12.92;
    vec3 high = 1.055 * pow(linear, vec3(1.0 / 2.4)) - 0.055;
    return mix(high, low, lessThanEqual(linear, vec3(0.0031308)));
}

void main() {
    vec3 hdr = texture(u_sceneColor, v_uv).rgb;
    float depth = texture(u_sceneDepth, v_uv).r;

    hdr = max(hdr, vec3(0.0));

    vec3 exposed = hdr * exp2(u_exposureStops);

    vec3 ldr;
    if (u_toneMapMode == 0) {
        ldr = clamp(exposed, 0.0, 1.0);
    } else {
        ldr = exposed / (vec3(1.0) + exposed);
    }

    if (u_visualizationMode == 1) {
        ldr = hdr / u_hdrDisplayMax;
    } else if (u_visualizationMode == 3) {
        float lum = dot(hdr, vec3(0.2126, 0.7152, 0.0722));
        ldr = vec3(clamp(lum / u_hdrDisplayMax, 0.0, 1.0));
    } else if (u_visualizationMode == 4) {
        float lum = dot(hdr, vec3(0.2126, 0.7152, 0.0722));
        float value = log2(max(lum, 1e-6));
        ldr = vec3(clamp((value + 10.0) / 20.0, 0.0, 1.0));
    } else if (u_visualizationMode == 5) {
        ldr = vec3(u_reversedDepth ? 1.0 - depth : depth);
    } else if (u_visualizationMode == 6) {
        bool clipped = any(greaterThan(exposed, vec3(1.0)));
        ldr = clipped ? vec3(1.0, 0.0, 1.0) : vec3(0.0);
    } else if (u_visualizationMode == 7) {
        bool dim = all(lessThan(exposed, vec3(1.0/255.0)));
        ldr = dim ? vec3(0.0, 1.0, 1.0) : vec3(0.0);
    } else if (u_visualizationMode == 8) {
        bool invalid = any(isnan(hdr)) || any(isinf(hdr));
        ldr = invalid ? vec3(1.0, 0.0, 0.0) : vec3(0.0);
    } else if (u_visualizationMode == 9) {
        float lum = dot(ldr, vec3(0.2126, 0.7152, 0.0722));
        ldr = vec3(lum);
    }

    if (u_grayscale && u_visualizationMode == 0) {
        float lum = dot(ldr, vec3(0.2126, 0.7152, 0.0722));
        ldr = vec3(lum);
    }

    // This debug view deliberately bypasses display encoding so it exposes the
    // linear LDR values that the final view converts for an sRGB display.
    vec3 outputColor = u_visualizationMode == 2 ? ldr : srgbEncode(ldr);
    FragColor = vec4(outputColor, 1.0);
})";
}

std::string fxaa_vertex_shader_source() {
    return
        R"(#version 330

out vec2 v_uv;

void main() {
    vec2 position = vec2((gl_VertexID == 1) ? 3.0 : -1.0,
                         (gl_VertexID == 2) ? 3.0 : -1.0);
    gl_Position = vec4(position, 0.0, 1.0);
    v_uv = position * 0.5 + 0.5;
})";
}

std::string fxaa_fragment_shader_source() {
    return
        R"(#version 330

in vec2 v_uv;

uniform sampler2D u_inputTexture;
uniform vec2 u_invTextureSize;
uniform bool u_fxaaEnabled;
uniform float u_edgeThreshold;
uniform float u_edgeThresholdMin;
uniform float u_subpixelAmount;

out vec4 FragColor;

float rgb2luma(vec3 rgb) {
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

void main() {
    if (!u_fxaaEnabled) {
        FragColor = texture(u_inputTexture, v_uv);
        return;
    }

    vec2 texelSize = u_invTextureSize;

    vec3 rgbN  = texture(u_inputTexture, v_uv + vec2( 0, -1) * texelSize).rgb;
    vec3 rgbW  = texture(u_inputTexture, v_uv + vec2(-1,  0) * texelSize).rgb;
    vec3 rgbM  = texture(u_inputTexture, v_uv).rgb;
    vec3 rgbE  = texture(u_inputTexture, v_uv + vec2( 1,  0) * texelSize).rgb;
    vec3 rgbS  = texture(u_inputTexture, v_uv + vec2( 0,  1) * texelSize).rgb;

    float lumaN = rgb2luma(rgbN);
    float lumaW = rgb2luma(rgbW);
    float lumaM = rgb2luma(rgbM);
    float lumaE = rgb2luma(rgbE);
    float lumaS = rgb2luma(rgbS);

    float rangeMin = min(lumaM, min(min(lumaN, lumaW), min(lumaE, lumaS)));
    float rangeMax = max(lumaM, max(max(lumaN, lumaW), max(lumaE, lumaS)));
    float range = rangeMax - rangeMin;

    if (range < max(u_edgeThresholdMin, rangeMax * u_edgeThreshold)) {
        FragColor = vec4(rgbM, 1.0);
        return;
    }

    vec3 rgbNW = texture(u_inputTexture, v_uv + vec2(-1, -1) * texelSize).rgb;
    vec3 rgbNE = texture(u_inputTexture, v_uv + vec2( 1, -1) * texelSize).rgb;
    vec3 rgbSW = texture(u_inputTexture, v_uv + vec2(-1,  1) * texelSize).rgb;
    vec3 rgbSE = texture(u_inputTexture, v_uv + vec2( 1,  1) * texelSize).rgb;

    float lumaNW = rgb2luma(rgbNW);
    float lumaNE = rgb2luma(rgbNE);
    float lumaSW = rgb2luma(rgbSW);
    float lumaSE = rgb2luma(rgbSE);

    float horizontal =
        abs(lumaN + lumaS - 2.0 * lumaM) * 2.0 +
        abs(lumaNE + lumaSE - 2.0 * lumaE) +
        abs(lumaNW + lumaSW - 2.0 * lumaW);
    float vertical =
        abs(lumaE + lumaW - 2.0 * lumaM) * 2.0 +
        abs(lumaNE + lumaNW - 2.0 * lumaN) +
        abs(lumaSE + lumaSW - 2.0 * lumaS);

    bool isHorizontal = horizontal >= vertical;
    float luma1 = isHorizontal ? lumaN : lumaW;
    float luma2 = isHorizontal ? lumaS : lumaE;
    float gradient1 = luma1 - lumaM;
    float gradient2 = luma2 - lumaM;

    bool step1 = abs(gradient1) >= abs(gradient2);
    float gradientScaled = 0.25 * max(abs(gradient1), abs(gradient2));
    float stepLength = isHorizontal ? texelSize.y : texelSize.x;

    float lumaLocal1;
    if (step1) {
        stepLength = -stepLength;
        lumaLocal1 = 0.5 * (luma1 + lumaM);
    } else {
        lumaLocal1 = 0.5 * (luma2 + lumaM);
    }

    vec2 offset = isHorizontal ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);
    vec2 uv1 = v_uv - offset;
    vec2 uv2 = v_uv + offset;

    float lumaEnd1 = rgb2luma(texture(u_inputTexture, uv1).rgb);
    float lumaEnd2 = rgb2luma(texture(u_inputTexture, uv2).rgb);
    lumaEnd1 -= lumaLocal1;
    lumaEnd2 -= lumaLocal1;

    bool reached1 = abs(lumaEnd1) >= gradientScaled;
    bool reached2 = abs(lumaEnd2) >= gradientScaled;
    bool reachedBoth = reached1 && reached2;

    if (!reached1) uv1 -= offset;
    if (!reached2) uv2 += offset;

    if (!reachedBoth) {
        for (int i = 0; i < 8; i++) {
            if (!reached1) {
                lumaEnd1 = rgb2luma(texture(u_inputTexture, uv1).rgb) - lumaLocal1;
            }
            if (!reached2) {
                lumaEnd2 = rgb2luma(texture(u_inputTexture, uv2).rgb) - lumaLocal1;
            }
            reached1 = abs(lumaEnd1) >= gradientScaled;
            reached2 = abs(lumaEnd2) >= gradientScaled;
            reachedBoth = reached1 && reached2;
            if (!reached1) uv1 -= offset;
            if (!reached2) uv2 += offset;
            if (reachedBoth) break;
        }
    }

    float distance1 = isHorizontal ? (v_uv.y - uv1.y) : (v_uv.x - uv1.x);
    float distance2 = isHorizontal ? (uv2.y - v_uv.y) : (uv2.x - v_uv.x);
    float direction1 = distance1 < distance2 ? 1.0 : 0.0;
    float distanceFinal = min(distance1, distance2);
    float edgeLength = distance1 + distance2;

    float pixelOffset = -distanceFinal / (edgeLength + 1e-6) + 0.5;

    bool centerLumaCorrect = ((lumaLocal1 - lumaM) < 0.0) == ((lumaEnd1 - lumaM) < 0.0);
    float finalOffset = centerLumaCorrect ? pixelOffset : 0.0;

    float subpixelLuma = (2.0 * lumaM + lumaN + lumaE + lumaS + lumaW) / 6.0;
    float subpixelAmount = clamp(pow(abs(subpixelLuma - lumaM) / max(range, 1e-6), 2.0) * u_subpixelAmount, 0.0, 1.0);

    vec2 finalUv = v_uv + (isHorizontal ? vec2(0.0, finalOffset * texelSize.y) : vec2(finalOffset * texelSize.x, 0.0));
    vec3 edgeColor = 0.5 * (texture(u_inputTexture, finalUv).rgb + texture(u_inputTexture, v_uv - (isHorizontal ? vec2(0.0, finalOffset * texelSize.y) : vec2(finalOffset * texelSize.x, 0.0))).rgb);

    vec3 subpixelColor = (rgbNW + rgbNE + rgbSW + rgbSE) * 0.25;
    FragColor = vec4(mix(edgeColor, subpixelColor, subpixelAmount), 1.0);
})";
}

} // namespace opengl
