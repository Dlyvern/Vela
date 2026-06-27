#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inTextureCoordinates;

layout(set = 0, binding = 0) uniform sampler2D inAlbedo;

void main()
{
    outColor = texture(inAlbedo, inTextureCoordinates);
}