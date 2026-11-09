#version 450

layout(location = 0) in vec2 inTextureCoordinates;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D inFont;

// ImGui colors are sRGB-encoded, but Vela attachments are linear and presenting encodes to sRGB.
// Without this the UI would be encoded twice and look washed out.
vec3 srgbToLinear(vec3 color)
{
    return mix(color / 12.92, pow((color + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), color));
}

void main()
{
    vec4 color = inColor * texture(inFont, inTextureCoordinates);
    outColor = vec4(srgbToLinear(color.rgb), color.a);
}
