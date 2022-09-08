#include "common.h"

struct Appdata
{
    float4 Position     : POSITION;
    float2 Uv           : TEXCOORD0;
    float4 Color        : COLOR0;
};

struct VertexOutput
{
    float4 HPosition    : POSITION;

    float2 Uv           : TEXCOORD0;
    float4 Color        : COLOR0;

#if defined(PIN_FOG)
    float FogFactor     : TEXCOORD1;
#endif
};

uniform float4 UIParams;  // x = luminance sampling on/off, w = z offset
TEX_DECLARE2D(DiffuseMap, 0);

VertexOutput UIVS(Appdata IN)
{
    VertexOutput OUT = (VertexOutput)0;

    OUT.HPosition = mul(G(ViewProjection), IN.Position);
    OUT.HPosition.z -= UIParams.w; // against z-fighting

    OUT.Uv = IN.Uv;
    OUT.Color = IN.Color;

#if defined(PIN_FOG)
    OUT.FogFactor = (G(FogParams).z - OUT.HPosition.w) * G(FogParams).w;
#endif

    return OUT;
}

float4 UIPS(VertexOutput IN): COLOR0
{
    float4 base;

    if (UIParams.x > 0.5)
        base = float4(1, 1, 1,tex2D(DiffuseMap, IN.Uv).r);
    else
        base = tex2D(DiffuseMap, IN.Uv);

    float4 result = IN.Color * base;

#if defined(PIN_FOG)
    result.rgb = lerp(G(FogColor), result.rgb, saturate(IN.FogFactor));
#endif

   return result;
}
