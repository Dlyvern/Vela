#version 450

layout(location = 0) in vec2 inTextureCoordinates;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inLightSpacePosition;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D inAlbedo;
layout(set = 2, binding = 0) uniform sampler2D inShadow;

void main()
{
    const vec3 lightDirection =
        normalize(vec3(0.4, 1.0, 0.3));

    const vec3 lightColor =
        vec3(1.0, 0.98, 0.92);

    const float ambient = 0.25;

    vec3 normal = normalize(inNormal);

    float diffuse =
        max(dot(normal, lightDirection), 0.0);

    vec4 albedo =
        texture(inAlbedo, inTextureCoordinates);

    vec3 shadowCoord =
        inLightSpacePosition.xyz /
        inLightSpacePosition.w;

    shadowCoord.xy =
        shadowCoord.xy * 0.5 + 0.5;

    float shadowDepth =
        texture(inShadow, shadowCoord.xy).r;

    float currentDepth =
        shadowCoord.z;

    const float bias = 0.005;

    float shadow = 1.0;

    bool insideLightFrustum =
        shadowCoord.x >= 0.0 && shadowCoord.x <= 1.0 &&
        shadowCoord.y >= 0.0 && shadowCoord.y <= 1.0 &&
        shadowCoord.z >= 0.0 && shadowCoord.z <= 1.0;

    if (insideLightFrustum && currentDepth - bias > shadowDepth)
    {
        shadow = 0.0;
    }

    float lighting =
        ambient +
        diffuse * shadow;

    outColor =
        vec4(
            albedo.rgb * lightColor * lighting,
            albedo.a
        );
}