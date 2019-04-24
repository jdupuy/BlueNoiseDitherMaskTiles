

layout (r32f) uniform coherent image2D u_DitherSampler;
uniform ivec2 u_ImageResolution;
uniform float u_Value = 1.0f;

layout(std430, binding = 0)
buffer DensityBuffer {
    ivec4 u_DataBuffer[];
};

#ifdef VERTEX_SHADER
layout(location = 0) flat out ivec2 o_P;

void main(void)
{
    vec2 u = vec2(gl_VertexID & 1, gl_VertexID >> 1 & 1);

    o_P = u_DataBuffer[1].xy; // position
    gl_Position = vec4(2.0 * u - 1.0, 0.0, 1.0);

    // write sample to dither texture
    if (gl_VertexID == 0) {
        imageStore(u_DitherSampler, o_P, vec4(u_Value));
        int offset = u_DataBuffer[1].z;
        u_DataBuffer[offset].w = 1; // mark sample as used
    }
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) flat in ivec2 i_P;
layout(location = 0) out vec4 o_FragColor;

void main(void)
{
    ivec2 P = ivec2(gl_FragCoord.xy);
    ivec2 Pref = i_P;
    ivec2 zAbs = abs(P - Pref);
    vec2 z = vec2(min(zAbs, u_ImageResolution - zAbs));
    float zSqr = dot(z, z);
    float w = exp(- zSqr / (2.1 * 2.1));

    o_FragColor = vec4(w);
}
#endif
