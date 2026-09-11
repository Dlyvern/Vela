#version 450

struct Particle
{
    vec2 position;
    vec2 velocity;
    vec4 color;
};

layout(std430, set = 1, binding = 0) readonly buffer Particles
{
    Particle particles[];
} particleBuffer;

layout(location = 0) out vec4 outColor;

void main()
{
    const vec2 corners[6] = vec2[6](
        vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
        vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

    Particle particle = particleBuffer.particles[gl_InstanceIndex];

    outColor = particle.color;

    gl_Position = vec4(particle.position + corners[gl_VertexIndex] * 0.008, 0.0, 1.0);
}
