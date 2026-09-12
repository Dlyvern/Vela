#version 450

layout(push_constant) uniform ShadowPC
{
    mat4 lightModel;
} shadowPc;

layout(location = 0) in vec3 inPosition;

void main()
{
    gl_Position = shadowPc.lightModel * vec4(inPosition, 1.0);
}
