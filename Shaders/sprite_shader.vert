#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTextureCoordinates;

layout(location = 0) out vec2 outTextureCoordinates;

layout(set = 0, binding = 1) uniform MVP
{
    mat4 model;
    mat4 view;
    mat4 projection;
} mvp;

void main() 
{
    outTextureCoordinates = inTextureCoordinates;
    
    gl_Position = mvp.projection * mvp.view * mvp.model * vec4(inPosition, 0.0, 1.0);
}