
uniform sampler2D u_DitherSampler;

#ifdef VERTEX_SHADER
layout(location = 0) out vec2 o_TexCoord;

void main(void)
{
    o_TexCoord  = vec2(gl_VertexID & 1, gl_VertexID >> 1 & 1);
    gl_Position = vec4(2.0 * o_TexCoord - 1.0, 0.0, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) in vec2 i_TexCoord;
layout(location = 0) out vec4 o_FragColor;

void main(void)
{
    o_FragColor = texture(u_DitherSampler, i_TexCoord).rrrr;
}
#endif
