#define PIN_SURFACE
#include "default.hlsl"

#ifndef CFG_WANG_TILES
float4 sampleFar(TEXTURE_IN_2D(s), float2 uv, float fade, float cutoff)
{
#ifdef GLSLES
    return tex2D(s, uv);
#else
    if (cutoff == 0)
        return tex2D(s, uv);
    else
    {
        float cscale = 1 / (1 - cutoff);

        return lerp(tex2D(s, uv * (CFG_FAR_TILING) ), tex2D(s, uv ), saturate0(fade * cscale - cutoff * cscale));
    }
#endif
}
#endif

Surface surfaceShader(SurfaceInput IN, float2 fade2)
{
#ifdef CFG_WANG_TILES
    float2 wangUv;
    float4 wangUVDerivatives;
    getWang(TEXTURE_WANG(NormalDetailMap), IN.Uv, CFG_TEXTURE_TILING, wangUv, wangUVDerivatives);
#endif

    float2 uv = IN.Uv * (CFG_TEXTURE_TILING);
	
	float fadeDiffuse = fade2.x;
	float fade = fade2.y;

#ifdef CFG_OPT_DIFFUSE_CONST
    float4 diffuse = 1;
#else 
 
    #ifdef CFG_WANG_TILES
        float4 diffuse = sampleWang(TEXTURE(DiffuseMap), wangUv, wangUVDerivatives);
    #else
        float4 diffuse = sampleFar(TEXTURE(DiffuseMap), uv, fadeDiffuse, CFG_FAR_DIFFUSE_CUTOFF);
    #endif
   
   diffuse.rgba = diffuse.rgba * (CFG_DIFFUSE_SCALE);

#endif

#ifdef CFG_OPT_NORMAL_CONST
    float3 normal = float3(0, 0, 1);
#else
    #ifdef CFG_WANG_TILES
        float3 normal = nmapUnpack(sampleWang(TEXTURE(NormalMap), wangUv, wangUVDerivatives));
    #else
        float3 normal = nmapUnpack(sampleFar(TEXTURE(NormalMap), uv, fade, CFG_FAR_NORMAL_CUTOFF));
    #endif
#endif

#ifndef GLSLES
    #ifndef CFG_WANG_TILES // normal detail unavailable when running wang tiles
        float3 normalDetail = nmapUnpack(tex2D(NormalDetailMap, uv * (CFG_NORMAL_DETAIL_TILING)));
        normal.xy += normalDetail.xy * (CFG_NORMAL_DETAIL_SCALE);
    #endif
#endif

    normal.xy *= fade;

    float shadowFactor = 1 + normal.x * (CFG_NORMAL_SHADOW_SCALE);

#ifdef CFG_OPT_BLEND_COLOR
	float3 albedo = lerp(float3(1, 1, 1), IN.Color.rgb, diffuse.a) * diffuse.rgb * shadowFactor;
#else
    float3 albedo = IN.Color.rgb * diffuse.rgb * shadowFactor;
#endif

#ifndef GLSLES
    float4 studs = tex2D(StudsMap, IN.UvStuds);

    albedo *= studs.r * 2;
#endif

#ifdef CFG_WANG_TILES
    float2 specular = sampleWang(TEXTURE(SpecularMap), wangUv, wangUVDerivatives).rg;
#else
    float2 specular = sampleFar(TEXTURE(SpecularMap), uv, fade, CFG_FAR_SPECULAR_CUTOFF).rg;
#endif

    // make sure glossiness is never 0 to avoid fp specials
    float2 specbase = specular * float2(CFG_SPECULAR_SCALE, CFG_GLOSS_SCALE) + float2(0, 0.01);
    float2 specfade = lerp(float2(CFG_SPECULAR_LOD, CFG_GLOSS_LOD), specbase, fade);

    Surface surface = (Surface)0;
    surface.albedo = albedo;
    surface.normal = normal;
    surface.specular = specfade.r;
    surface.gloss = specfade.g;
    surface.reflectance = specular.g * fade * (CFG_REFLECTION_SCALE);

	return surface;
}
