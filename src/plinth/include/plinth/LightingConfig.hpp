#ifndef PLINTH_LIGHTINGCONFIG_HPP
#define PLINTH_LIGHTINGCONFIG_HPP

#include <linal/vec.hpp>

namespace renderer {

enum class MeshCullFaceMode {
    back,
    front,
    none,
};

struct LightingConfig {
    linal::float3 lightPosition{0.0F, 0.0F, 10.0F};
    linal::float3 lightColor{1.0F, 1.0F, 1.0F};
    linal::float3 fillLightDir{-0.45F, 0.60F, 0.35F};
    linal::float3 fillLightColor{0.2F, 0.2F, 0.3F};
    linal::float3 ambientColor{0.1F, 0.1F, 0.1F};
    float shininess{8.0F};
    linal::float3 lightAttenuation{1.0F, 0.0F, 0.0F};
    linal::float3 materialAmbient{1.0F, 1.0F, 1.0F};
    linal::float3 materialDiffuse{1.0F, 1.0F, 1.0F};
    linal::float3 materialSpecular{1.0F, 1.0F, 1.0F};
};

} // namespace renderer

#endif // PLINTH_LIGHTINGCONFIG_HPP
