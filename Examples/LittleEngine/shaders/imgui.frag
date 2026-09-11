#version 450

layout(location = 0) in vec2 inTextureCoordinates;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D inFont;

void main()
{
    outColor = inColor * texture(inFont, inTextureCoordinates);
}