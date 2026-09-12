#version 450

layout(push_constant) uniform ModelPC
{
    mat4 model;
} modelPc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTextureCoordinates;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 outTextureCoordinates;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outLightSpacePosition;

layout(set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
} camera;

layout(set = 1, binding = 1) readonly buffer Light
{
    mat4 lightViewProjection;
} light;

void main()
{
    vec4 worldPosition = modelPc.model * vec4(inPosition, 1.0);

    outTextureCoordinates = inTextureCoordinates;
    outNormal = mat3(modelPc.model) * inNormal;
    outLightSpacePosition = light.lightViewProjection * worldPosition;

    gl_Position = camera.projection * camera.view * worldPosition;
}
