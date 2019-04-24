
uniform uint u_PassID;
uniform uint u_RandomOffset;
uniform sampler2D u_DensitySampler;

layout(std430, binding = 0)
buffer DensityBuffer {
    ivec4 u_DataBuffer[];
};

uint irng(uint rng_state)
{
    // LCG values from Numerical Recipes
    return 1664525u * rng_state + 1013904223u;
}

#ifdef COMPUTE_SHADER
layout (local_size_x = 1,
        local_size_y = 1,
        local_size_z = 1) in;

void main(void)
{
    uint cnt = (1u << u_PassID);
    uint threadID = gl_GlobalInvocationID.x;
    uint pos = cnt + ((threadID + u_RandomOffset) % cnt);
    uint b = irng(u_RandomOffset) & 1u;
    ivec4 d1 = u_DataBuffer[pos * 2 +     b];
    ivec4 d2 = u_DataBuffer[pos * 2 + 1 - b];
    float f1 = texelFetch(u_DensitySampler, d1.xy, 0).r;
    float f2 = texelFetch(u_DensitySampler, d2.xy, 0).r;

    if (d1.w == 0 && d2.w == 0) {
        u_DataBuffer[pos] = f1 < f2 ? d1 : d2;
    } else if (d1.w == 0) {
        u_DataBuffer[pos] = d1;
    } else {
        u_DataBuffer[pos] = d2;
    }
}
#endif

