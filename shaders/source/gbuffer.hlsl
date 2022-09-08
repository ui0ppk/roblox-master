#include "common.h"

// .xy = gbuffer width/height, .zw = inverse gbuffer width/height
uniform float4 TextureSize;

TEX_DECLARE2D(tex, 0);

struct v2f
{
    float4 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

#if defined(GLSL) || defined(DX11)
float4 convertPosition(float4 p)
{
	return p;
}
#else
float4 convertPosition(float4 p)
{
	// half-pixel offset
	return p + float4(-TextureSize.z, TextureSize.w, 0, 0);
}
#endif

#if defined(GLSL)
float2 convertUv(float4 p)
{
	return p.xy * 0.5 + 0.5;
}
#else
float2 convertUv(float4 p)
{
	return p.xy * float2(0.5, -0.5) + 0.5;
}
#endif


v2f gbufferVS( in float4 pos : POSITION )
{
    v2f o;
    o.pos = convertPosition(pos);
    o.uv =  convertUv(pos);
    return o;
}


float4 gbufferPS( v2f i ) : COLOR0
{
    return tex2D( tex, i.uv );
}
