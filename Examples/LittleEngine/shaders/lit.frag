#version 450

layout(location = 0) in vec2 inTextureCoordinates;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D inAlbedo;

void main()
{
    const vec3 lightDirection = normalize(vec3(0.4, 1.0, 0.3));
    const vec3 lightColor = vec3(1.0, 0.98, 0.92);
    const float ambient = 0.25;

    vec3 normal = normalize(inNormal);
    float diffuse = max(dot(normal, lightDirection), 0.0);

    vec4 albedo = texture(inAlbedo, inTextureCoordinates);

    outColor = vec4(albedo.rgb * lightColor * (ambient + diffuse), albedo.a);
}
