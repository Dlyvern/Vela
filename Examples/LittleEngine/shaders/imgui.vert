#version 450

layout(push_constant) uniform ModelPC
{
    mat4 model;
} modelPc;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTextureCoordinates;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 outTextureCoordinates;
layout(location = 1) out vec4 outColor;

void main()
{
    outTextureCoordinates = inTextureCoordinates;
    outColor = inColor;

    gl_Position = modelPc.model * vec4(inPosition, 0.0, 1.0);
}
