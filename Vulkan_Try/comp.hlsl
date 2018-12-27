#include "SharedConst.h"

struct Particle
{
    float2 pos;
    float2 vel;
};

// [[vk::binding(0)]]
cbuffer UniformBuffer : register(c0) {
    float2 windowSize;
    float dt;
    float dummy;
};

// [[vk::binding(0)]]
RWStructuredBuffer<Particle> part : register(u1);

[numthreads(64, 1, 1)]
void main(uint3 id : SV_TheadInvocationID)
{
    float2 acc = float2(0, 0);
    for (uint i = 0; i < PARTICLE_COUNT; i++)
    {
        float2 diff = part[i].pos - part[id.x].pos;
        float len = max(1e-10, length(diff));
        float k = 1e-4 * (len - 0.5) / len;
        acc += k * diff;
    }

    float t = dt;
    part[id.x].pos += t * part[id.x].vel + 0.5 * t * t * acc;
    part[id.x].vel += t * acc;
    part[id.x].vel *= 0.97;
}
