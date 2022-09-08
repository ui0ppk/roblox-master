#include "common.h"



struct Appdata
{
    float4 Position	    : POSITION;
    float2 Uv	        : TEXCOORD0;
    float4 Color        : COLOR0;
};

struct VertexOutput
{
    float4 HPosition    : POSITION;
    float2 Uv           : TEXCOORD0;
    float4 Color        : COLOR0;
};

VertexOutput ProfilerVS(Appdata IN)
{
    VertexOutput OUT = (VertexOutput)0;

    OUT.HPosition = mul(G(ViewProjection), IN.Position);
	OUT.HPosition.y = -OUT.HPosition.y;

	OUT.Uv = IN.Uv;
	OUT.Color = IN.Color;

    return OUT;
}

TEX_DECLARE2D(DiffuseMap, 0);

float4 ProfilerPS(VertexOutput IN): COLOR0
{
	float4 c0 = tex2D(DiffuseMap, IN.Uv);
	float4 c1 = tex2D(DiffuseMap, IN.Uv + float2(0, 1.f / 9.f));

    return c0.a < 0.5 ? float4(0, 0, 0, c1.a) : c0 * IN.Color;
}
