#include "common.h"

// Tunables
#define CFG_TEXTURE_TILING 0.2
#define CFG_TEXTURE_DETILING 0.1

#define CFG_SPECULAR 2
#define CFG_GLOSS 900

#define CFG_NORMAL_STRENGTH 0.25

#define CFG_REFRACTION_STRENGTH 0.05

#define CFG_FRESNEL_OFFSET 0.3

#define CFG_SSR_STEPS 8
#define CFG_SSR_START_DISTANCE 1
#define CFG_SSR_STEP_CLAMP 0.2
#define CFG_SSR_DEPTH_CUTOFF 10

// Shader code
struct Appdata
{
	ATTR_INT4 Position	: POSITION;

    ATTR_INT4 Normal	: NORMAL;

    ATTR_INT4 Material0	: TEXCOORD0;
    ATTR_INT4 Material1	: TEXCOORD1;
};

struct VertexOutput
{
    float4 HPosition    : POSITION;

    float4 Weights_Wave: COLOR0;
    float3 Tangents: COLOR1;

    float2 Uv0: TEXCOORD0;
    float2 Uv1: TEXCOORD1;
    float2 Uv2: TEXCOORD2;

    float4 LightPosition_Fog : TEXCOORD3;

	float3 Normal: TEXCOORD4;
    float4 View_Depth: TEXCOORD5;

#ifdef PIN_HQ
	float4 PositionScreen: TEXCOORD6;
#endif
};

WORLD_MATRIX_ARRAY(WorldMatrixArray, 72);

uniform float4 WaveParams; // .x = frequency  .y = phase  .z = height  .w = lerp
uniform float4 WaterColor; // deep water color
uniform float4 WaterParams; // .x = refraction depth scale, .y = refraction depth offset

float3 displacePosition(float3 position, float waveFactor)
{
	float x = sin((position.z - position.x) * WaveParams.x - WaveParams.y);
	float z = sin((position.z + position.x) * WaveParams.x + WaveParams.y);
	float p = (x + z) * WaveParams.z;

	float3 result = position;
	
	result.y += p * waveFactor;

	return result;
}

float4 clipToScreen(float4 pos)
{
#ifdef GLSL
	pos.xy = pos.xy * 0.5 + 0.5 * pos.w;
#else
	pos.xy = pos.xy * float2(0.5, -0.5) + 0.5 * pos.w;
#endif
	return pos;
}

float2 getUV(float3 position, ATTR_INT projection, float seed)
{
	float3 u = WorldMatrixArray[1 + int(projection)].xyz;
	float3 v = WorldMatrixArray[19 + int(projection)].xyz;

    float2 uv = float2(dot(position, u), dot(position, v)) * (0.25 * CFG_TEXTURE_TILING) + CFG_TEXTURE_DETILING * float2(seed, floor(seed * 2.6651441f));

    return uv;
}

VertexOutput WaterVS(Appdata IN)
{
    VertexOutput OUT = (VertexOutput)0;
    
	float3 posWorld = IN.Position.xyz * WorldMatrixArray[0].w + WorldMatrixArray[0].xyz;
    float3 normalWorld = IN.Normal.xyz * (1.0 / 127.0) - 1.0;

#if defined(GLSLES) && !defined(GL3) // iPad2 workaround
    float3 weights = abs(IN.Position.www - float3(0, 1, 2)) < 0.1;
#else
    float3 weights = IN.Position.www == float3(0, 1, 2);
#endif

    float waveFactor = dot(weights, IN.Material0.xyz) * (1.0 / 255.0);

#ifdef PIN_HQ
	float fade = saturate0(1 - dot(posWorld - G(CameraPosition), -G(ViewDir).xyz) * G(FadeDistance_GlowFactor).y);

	posWorld = displacePosition(posWorld, waveFactor * fade);
#endif

	OUT.HPosition = mul(G(ViewProjection), float4(posWorld, 1));

    OUT.LightPosition_Fog = float4(lgridPrepareSample(lgridOffset(posWorld, normalWorld)), (G(FogParams).z - OUT.HPosition.w) * G(FogParams).w);

    OUT.Uv0 = getUV(posWorld, IN.Material1.x, IN.Normal.w);
    OUT.Uv1 = getUV(posWorld, IN.Material1.y, IN.Material0.w);
    OUT.Uv2 = getUV(posWorld, IN.Material1.z, IN.Material1.w);

    OUT.Weights_Wave.xyz = weights;
    OUT.Weights_Wave.w = waveFactor;

	OUT.Normal = normalWorld;
    OUT.View_Depth = float4(G(CameraPosition) - posWorld, OUT.HPosition.w);
	OUT.Tangents = float3(IN.Material1.xyz) > 7.5; // side vs top

#ifdef PIN_HQ
	OUT.PositionScreen = clipToScreen(OUT.HPosition);
#endif

	return OUT;
}

TEX_DECLARE2D(NormalMap1, 0);
TEX_DECLARE2D(NormalMap2, 1);
TEX_DECLARECUBE(EnvMap, 2);
LGRID_SAMPLER(LightMap, 3);
TEX_DECLARE2D(LightMapLookup, 4);
TEX_DECLARE2D(GBufferColor, 5);
TEX_DECLARE2D(GBufferDepth, 6);

float fresnel(float ndotv)
{
	return saturate(0.78 - 2.5 * abs(ndotv)) + CFG_FRESNEL_OFFSET;
}

float4 sampleMix(float2 uv)
{
#ifdef PIN_HQ
	return lerp(tex2D(NormalMap1, uv), tex2D(NormalMap2, uv), WaveParams.w);
#else
	return tex2D(NormalMap1, uv);
#endif
}

float3 sampleNormal(float2 uv0, float2 uv1, float2 uv2, float3 w, float3 normal, float3 tsel)
{
	return terrainNormal(sampleMix(uv0), sampleMix(uv1), sampleMix(uv2), w, normal, tsel);
}

float3 sampleNormalSimple(float2 uv0, float2 uv1, float2 uv2, float3 w)
{
    float4 data = sampleMix(uv0) * w.x + sampleMix(uv1) * w.y + sampleMix(uv2) * w.z;

    return nmapUnpack(data).xzy;
}

float unpackDepth(float2 uv)
{
	float4 geomTex = tex2D(GBufferDepth, uv);
	float d = geomTex.z * (1.0f/256.0f) + geomTex.w;
	return d * GBUFFER_MAX_DEPTH;
}

float3 getRefractedColor(float4 cpos, float3 N, float3 waterColor)
{
    float2 refruv0 = cpos.xy / cpos.w;
    float2 refruv1 = refruv0 + N.xz * CFG_REFRACTION_STRENGTH;

    float4 refr0 = tex2D(GBufferColor, refruv0);
	refr0.w = unpackDepth(refruv0);

    float4 refr1 = tex2D(GBufferColor, refruv1);
	refr1.w = unpackDepth(refruv1);

	float4 result = lerp(refr0, refr1, saturate(refr1.w - cpos.w));

	// Estimate water absorption by a scaled depth difference
	float depthfade = saturate((result.w - cpos.w) * WaterParams.x + WaterParams.y);

	// Since GBuffer depth is clamped we tone the refraction down after half of the range for a smooth fadeout
	float gbuffade = saturate(cpos.w * (2.f / GBUFFER_MAX_DEPTH) - 1);

	float fade = saturate(depthfade + gbuffade);

    return lerp(result.rgb, waterColor, fade);
}

float3 getReflectedColor(float4 cpos, float3 wpos, float3 R)
{
    float3 result = 0;
    float inside = 0;

    float distance = CFG_SSR_START_DISTANCE;
    float diff = 0;
	float diffclamp = cpos.w * CFG_SSR_STEP_CLAMP;

	float4 Pproj = cpos;
	float4 Rproj = clipToScreen(mul(G(ViewProjection), float4(R, 0)));

#ifndef GLSL
	[unroll]
#endif
    for (int i = 0; i < CFG_SSR_STEPS; ++i)
    {
        distance += clamp(diff, -diffclamp, diffclamp);

        float4 cposi = Pproj + Rproj * distance;
        float2 uv = cposi.xy / cposi.w;
        float depth = unpackDepth(uv);

        diff = depth - cposi.w;
    }

    float4 cposi = Pproj + Rproj * distance;
    float2 uv = cposi.xy / cposi.w;

	// Ray hit has to be inside the screen bounds
    float ufade = abs(uv.x - 0.5) < 0.5;
    float vfade = abs(uv.y - 0.5) < 0.5;

	// Fade reflections out with distance; use max(ray hit, original depth) to discard hits that are too far
	// 4 - depth * 4 would give us fade out from 0.75 to 1; 3.9 makes sure reflections go to 0 slightly before GBUFFER_MAX_DEPTH
    float wfade = saturate((4 - 0.1) - max(cpos.w, cposi.w) * (4.f / GBUFFER_MAX_DEPTH));

	// Ray hit has to be reasonably close to where we started
    float dfade = abs(diff) < CFG_SSR_DEPTH_CUTOFF;

	// Avoid back-projection 
    float Vfade = Rproj.w > 0;

    float fade = ufade * vfade * wfade * dfade * Vfade;

    return lerp(texCUBE(EnvMap, R).rgb, tex2D(GBufferColor, uv).rgb, fade);
}

float4 WaterPS(VertexOutput IN): COLOR0
{
    float4 light = lgridSample(TEXTURE(LightMap), TEXTURE(LightMapLookup), IN.LightPosition_Fog.xyz);
	float shadow = light.a;

	float3 w = IN.Weights_Wave.xyz;

	// Use simplified normal reconstruction for LQ mobile (assumes flat water surface)
#if defined(GLSLES) && !defined(PIN_HQ)
	float3 normal = sampleNormalSimple(IN.Uv0, IN.Uv1, IN.Uv2, w);
#else
	float3 normal = sampleNormal(IN.Uv0, IN.Uv1, IN.Uv2, w, IN.Normal, IN.Tangents);
#endif

	// Flatten the normal for Fresnel and for reflections to make them less chaotic
	float3 flatNormal = lerp(IN.Normal, normal, CFG_NORMAL_STRENGTH);

	float3 waterColor = WaterColor.rgb;

#ifdef PIN_HQ
	float fade = saturate0(1 - IN.View_Depth.w * G(FadeDistance_GlowFactor).y);

	float3 view = normalize(IN.View_Depth.xyz);

	float fre = fresnel(dot(flatNormal, view)) * IN.Weights_Wave.w;

	float3 position = G(CameraPosition) - IN.View_Depth.xyz;

#ifdef PIN_GBUFFER
    float3 refr = getRefractedColor(IN.PositionScreen, normal, waterColor);
	float3 refl = getReflectedColor(IN.PositionScreen, position, reflect(-view, flatNormal));
#else
    float3 refr = waterColor;
	float3 refl = texCUBE(EnvMap, reflect(-view, flatNormal)).rgb;
#endif

    float specularIntensity = CFG_SPECULAR * fade;
    float specularPower = CFG_GLOSS;

	float3 specular = G(Lamp0Color) * (specularIntensity * shadow * (float)(half)pow(saturate(dot(normal, normalize(-G(Lamp0Dir) + view))), specularPower));
#else
	float3 view = normalize(IN.View_Depth.xyz);

	float fre = fresnel(dot(flatNormal, view));

    float3 refr = waterColor;

	float3 refl = texCUBE(EnvMap, reflect(-IN.View_Depth.xyz, flatNormal)).rgb;

	float3 specular = 0;
#endif

    // Combine
	float4 result;
    result.rgb = lerp(refr, refl, fre) * (G(AmbientColor).rgb + G(Lamp0Color).rgb * shadow + light.rgb) + specular;
    result.a = 1;

    float fogAlpha = saturate(IN.LightPosition_Fog.w);

    result.rgb = lerp(G(FogColor), result.rgb, fogAlpha);

	return result;
}
