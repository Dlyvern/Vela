#version 450

layout(push_constant) uniform ShadowPC
{
    mat4 lightModel;
} shadowPc;

layout(location = 0) in vec3 inPosition;

// The shadow pass draws meshes in the StaticVertex layout, which also declares uv (1) and normal (2).
// Declared so the pipeline's vertex input matches the shader; depth-only rendering ignores them.
layout(location = 1) in vec2 inTextureCoordinates;
layout(location = 2) in vec3 inNormal;

void main()
{
    gl_Position = shadowPc.lightModel * vec4(inPosition, 1.0);
}
