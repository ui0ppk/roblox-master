#if defined(PIN_HQ)
#define PIN_SURFACE
#include "default.hlsl"

#define CFG_TEXTURE_TILING              1

#define CFG_BUMP_INTENSITY              0.5

#define CFG_SPECULAR					0.4
#define CFG_GLOSS						9

#define CFG_NORMAL_SHADOW_SCALE         0.1

Surface surfaceShader(SurfaceInput IN, float2 fade2)
{
	float fade = fade2.y;
	
    float4 studs = tex2D(DiffuseMap, IN.UvStuds);
    float3 normal = nmapUnpack(tex2D(NormalMap, IN.UvStuds));

#ifdef GLSLES
    float3 noise = float3(0, 0, 1);
#else
    float3 noise = nmapUnpack(tex2D(NormalDetailMap, IN.Uv * (CFG_TEXTURE_TILING)));
#endif

    float noiseScale = saturate0(IN.Color.a * 2 * (CFG_BUMP_INTENSITY) - 1 * (CFG_BUMP_INTENSITY));

#ifdef PIN_REFLECTION
    noiseScale *= saturate(1 - 2 * IN.Reflectance);
#endif

    normal.xy += noise.xy * noiseScale;

    normal.xy *= fade;

    Surface surface = (Surface)0;
    surface.albedo = IN.Color.rgb * (studs.r * 2);
    surface.normal = normal;
    surface.specular = (CFG_SPECULAR);
    surface.gloss = (CFG_GLOSS);

#ifdef PIN_REFLECTION
    surface.reflectance = IN.Reflectance;
#endif

	return surface;
}
#else
#define PIN_PLASTIC
#include "default.hlsl"
#endif
