#ifndef GLSL
struct Globals
{
#endif
    float4x4 ViewProjection;

    float4 ViewRight;
    float4 ViewUp;
    float4 ViewDir;
    float3 CameraPosition;

    float3 AmbientColor;
    float3 Lamp0Color;
    float3 Lamp0Dir;
    float3 Lamp1Color;

    float3 FogColor;
    float4 FogParams;

    float4 LightBorder;
    float4 LightConfig0;
    float4 LightConfig1;
    float4 LightConfig2;
    float4 LightConfig3;

    float4 FadeDistance_GlowFactor;
    float4 OutlineBrightness_ShadowInfo;

	float4 ShadowMatrix0;
	float4 ShadowMatrix1;
	float4 ShadowMatrix2;
#ifndef GLSL
};

#ifdef DX11
cbuffer Globals: register( b0 ) { Globals _G; };
#else
uniform Globals _G: register(c0);
#endif

#define G(x) _G.x
#else
#define G(x) x
#endif
