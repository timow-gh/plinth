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

    out vec4 v_color;
    out vec3 v_normal;
    out vec3 v_position;

    void main() {
        gl_Position = u_projection * u_view * u_model * vec4(a_vertex, 1.0);
        v_color = a_color;
        v_normal = mat3(u_normalMatrix) * a_normal;
        v_position = vec3(u_model * vec4(a_vertex, 1.0));
    })";
}

std::string mesh_fragment_shader_source() {
    return
        R"(#version 330

    in vec4 v_color;
    in vec3 v_normal;
    in vec3 v_position;

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

    void main() {
        // Normalize input vectors
        vec3 norm = normalize(v_normal);
        vec3 lightDir = normalize(u_lightPos - v_position);
        vec3 viewDir = normalize(u_viewPos - v_position);

        // Ambient lighting
        vec3 ambient = u_ambientColor * v_color.rgb;

        // Diffuse lighting
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = u_lightColor * diff * v_color.rgb;

        // Directional fill lighting
        float fillDirLength = length(u_fillLightDirection);
        vec3 fillDir = fillDirLength > 0.0 ? u_fillLightDirection / fillDirLength : vec3(0.0);
        float fillDiff = max(dot(norm, fillDir), 0.0);
        vec3 fillDiffuse = u_fillLightColor * fillDiff * v_color.rgb;

        // Specular lighting
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_shininess);
        vec3 specular = u_lightColor * spec;

        // Combine results
        vec3 result = ambient + diffuse + fillDiffuse + specular;
        FragColor = vec4(result, v_color.a);
    })";
}

std::string post_processing_vertex_shader_source() {
    return
        R"(#version 330

out vec2 v_texCoord;

void main() {
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    v_texCoord = gl_Position.xy * 0.5 + 0.5;
})";
}

std::string post_processing_fragment_shader_source() {
    return
        R"(#version 330

in vec2 v_texCoord;
out vec4 FragColor;
uniform sampler2D u_sceneTexture;

void main() {
    FragColor = texture(u_sceneTexture, v_texCoord);
})";
}

} // namespace opengl
