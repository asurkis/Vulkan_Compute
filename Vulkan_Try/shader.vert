#version 450

layout (location = 0) in vec2 position;
layout (set = 0, binding = 0) uniform UniformBufferObject {
    vec2 windowSize;
    float dt;
    float dummy;
} ubo;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = vec4(position * min(ubo.windowSize.x, ubo.windowSize.y) / ubo.windowSize, 0.0, 1.0);
}
