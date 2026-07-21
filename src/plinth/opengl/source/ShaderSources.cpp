#include "OpenGL/ShaderSources.hpp"

namespace opengl {
std::string line_vertex_shader_source() {
    return
        R"(#version 330

uniform mat4 u_MVP;
uniform mat4 u_model;

in vec3 a_vertex;
in vec4 a_color;

out vec4 v_color;

void main() {
    gl_Position = u_MVP * u_model * vec4(a_vertex, 1.0);
    v_color = a_color;
})";
}

std::string line_fragment_shader_source() {
    return
        R"(#version 330

in vec4 v_color;

out vec4 FragColor;

void main() {
    FragColor = v_color;
}
)";
}
std::string point_color_vertex_shader_source() {
    return
        R"(#version 330

uniform mat4 u_MVP;
uniform mat4 u_model;
uniform float u_pointSize;

in vec3 a_vertex;
in vec4 a_color;

out vec4 v_color;

void main() {
    gl_Position = u_MVP * u_model * vec4(a_vertex, 1.0);
    gl_PointSize = u_pointSize;
    v_color = a_color;
})";
}
std::string point_color_fragment_shader_source() {
    return
        R"(#version 330

in vec4 v_color;

out vec4 FragColor;

void main() {
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

    void main() {
        // Normalize input vectors
        vec3 norm = normalize(v_normal);
        vec3 lightDir = normalize(u_lightPos - v_position);
        vec3 viewDir = normalize(u_viewPos - v_position);

        vec3 albedo = v_color.rgb;
        if (u_hasAlbedoTexture) {
            albedo *= texture(u_albedoTexture, v_texCoord).rgb;
        }

        // Ambient lighting
        vec3 ambient = u_ambientColor * albedo;

        // Diffuse lighting
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = u_lightColor * diff * albedo;

        // Directional fill lighting
        float fillDirLength = length(u_fillLightDirection);
        vec3 fillDir = fillDirLength > 0.0 ? u_fillLightDirection / fillDirLength : vec3(0.0);
        float fillDiff = max(dot(norm, fillDir), 0.0);
        vec3 fillDiffuse = u_fillLightColor * fillDiff * albedo;

        // Specular lighting
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_shininess);
        vec3 specular = u_lightColor * spec;

        // Combine results
        vec3 result = ambient + diffuse + fillDiffuse + specular;
        FragColor = vec4(result, v_color.a);
    })";
}

std::string presentation_vertex_shader_source() {
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

std::string presentation_fragment_shader_source() {
    return
        R"(#version 330

in vec2 v_uv;

uniform sampler2D u_sceneColor;

out vec4 FragColor;

void main() {
    FragColor = texture(u_sceneColor, v_uv);
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

uniform mat4 u_invProjection;

uniform bool   u_fogEnabled;
uniform int    u_fogMode;
uniform float  u_fogStart;
uniform float  u_fogEnd;
uniform float  u_fogDensity;
uniform vec3   u_fogColor;

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

    vec3 viewPos = vec3(0.0);
    if (u_fogEnabled) {
        vec4 clip = vec4(v_uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
        vec4 view = u_invProjection * clip;
        viewPos = view.xyz / view.w;
    }

    if (u_fogEnabled && depth < 0.9999) {
        float dist = length(viewPos);
        float fogAmount;
        if (u_fogMode == 0) {
            fogAmount = smoothstep(u_fogStart, u_fogEnd, dist);
        } else {
            fogAmount = 1.0 - exp(-u_fogDensity * dist);
        }
        hdr = mix(hdr, u_fogColor, fogAmount);
    }

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
        ldr = vec3(depth);
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

    vec3 encoded = srgbEncode(ldr);
    FragColor = vec4(encoded, 1.0);
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
