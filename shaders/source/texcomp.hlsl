#include "common.h"

struct Appdata
{
    float4 Position	    : POSITION;
    float2 Uv	        : TEXCOORD0;
};

struct VertexOutput
{
    float4 HPosition    : POSITION;
    float2 Uv           : TEXCOORD0;
};

VertexOutput TexCompVS(Appdata IN)
{
    VertexOutput OUT = (VertexOutput)0;

    OUT.HPosition = mul(G(ViewProjection), IN.Position);
    OUT.Uv = IN.Uv;

    return OUT;
}

TEX_DECLARE2D(DiffuseMap, 0);

uniform float4 Color;

float4 TexCompPS(VertexOutput IN): COLOR0
{
    return tex2Dbias(DiffuseMap, float4(IN.Uv, 0, -10)) * Color;
}

float4 TexCompPMAPS(VertexOutput IN): COLOR0
{
    float4 tex = tex2Dbias(DiffuseMap, float4(IN.Uv, 0, -10));
    
    return float4(tex.rgb * tex.a * Color.rgb, tex.a * Color.a);
}
