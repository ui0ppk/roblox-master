#include "common.h"

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

    float3 Weights: COLOR0;

    float4 Uv0: TEXCOORD0;
    float4 Uv1: TEXCOORD1;
    float4 Uv2: TEXCOORD2;

    float4 LightPosition_Fog : TEXCOORD3;
    float3 PosLightSpace     : TEXCOORD4;

#ifdef PIN_HQ
	float3 Normal: TEXCOORD5;
    float4 View_Depth: TEXCOORD6;

    float3 Tangents: COLOR1;
#else
    float3 Diffuse: COLOR1;
#endif
};

WORLD_MATRIX_ARRAY(WorldMatrixArray, 72);

uniform float4 LayerScale;

float4 getUV(float3 position, ATTR_INT material, ATTR_INT projection, float seed)
{
	float3 u = WorldMatrixArray[1 + int(projection)].xyz;
	float3 v = WorldMatrixArray[19 + int(projection)].xyz;

    float4 m = WorldMatrixArray[37 + int(material)];

    float2 uv = float2(dot(position, u), dot(position, v)) * m.x + m.y * float2(seed, floor(seed * 2.6651441f));

    return float4(uv, m.zw);
}

VertexOutput TerrainVS(Appdata IN)
{
    VertexOutput OUT = (VertexOutput)0;
    
	float3 posWorld = IN.Position.xyz * WorldMatrixArray[0].w + WorldMatrixArray[0].xyz;
    float3 normalWorld = IN.Normal.xyz * (1.0 / 127.0) - 1.0;

	OUT.HPosition = mul(G(ViewProjection), float4(posWorld, 1));

    OUT.LightPosition_Fog = float4(lgridPrepareSample(lgridOffset(posWorld, normalWorld)), (G(FogParams).z - OUT.HPosition.w) * G(FogParams).w);

    OUT.PosLightSpace = shadowPrepareSample(posWorld);

    OUT.Uv0 = getUV(posWorld, IN.Material0.x, IN.Material1.x, IN.Normal.w);
    OUT.Uv1 = getUV(posWorld, IN.Material0.y, IN.Material1.y, IN.Material0.w);
    OUT.Uv2 = getUV(posWorld, IN.Material0.z, IN.Material1.z, IN.Material1.w);

#if defined(GLSLES) && !defined(GL3) // iPad2 workaround
    OUT.Weights = abs(IN.Position.www - float3(0, 1, 2)) < 0.1;
#else
    OUT.Weights = IN.Position.www == float3(0, 1, 2);
#endif

#ifdef PIN_HQ
	OUT.Normal = normalWorld;
    OUT.View_Depth = float4(G(CameraPosition) - posWorld, OUT.HPosition.w);
	OUT.Tangents = float3(IN.Material1.xyz) > 7.5; // side vs top
#else
    float ndotl = dot(normalWorld, -G(Lamp0Dir));
    float3 diffuse = max(ndotl, 0) * G(Lamp0Color) + max(-ndotl, 0) * G(Lamp1Color);

    OUT.Diffuse = diffuse;
#endif

	return OUT;
}

TEX_DECLARE2D(AlbedoMap, 0);
TEX_DECLARE2D(NormalMap, 1);
TEX_DECLARE2D(SpecularMap, 2);
TEX_DECLARECUBE(EnvMap, 3);
LGRID_SAMPLER(LightMap, 4);
TEX_DECLARE2D(LightMapLookup, 5);
TEX_DECLARE2D(ShadowMap, 6);

float4 sampleMap(TEXTURE_IN_2D(s), float4 uv)
{
#ifdef PIN_HQ
    float2 uvs = uv.xy * LayerScale.xy;

    return tex2Dgrad(s, frac(uv.xy) * LayerScale.xy + uv.zw, ddx(uvs), ddy(uvs));
#else
    return tex2D(s, frac(uv.xy) * LayerScale.xy + uv.zw);
#endif
}

float4 sampleBlend(TEXTURE_IN_2D(s), float4 uv0, float4 uv1, float4 uv2, float3 w)
{
	return
        sampleMap(TEXTURE(s), uv0) * w.x + 
        sampleMap(TEXTURE(s), uv1) * w.y +
        sampleMap(TEXTURE(s), uv2) * w.z;
}

float3 sampleNormal(TEXTURE_IN_2D(s), float4 uv0, float4 uv1, float4 uv2, float3 w, float3 normal, float3 tsel)
{
	return terrainNormal(sampleMap(TEXTURE(s), uv0), sampleMap(TEXTURE(s), uv1), sampleMap(TEXTURE(s), uv2), w, normal, tsel);
}

void TerrainPS(VertexOutput IN,
#ifdef PIN_GBUFFER
    out float4 oColor1: COLOR1,
#endif
    out float4 oColor0: COLOR0)
{
    float4 light = lgridSample(TEXTURE(LightMap), TEXTURE(LightMapLookup), IN.LightPosition_Fog.xyz);
    float shadow = shadowSample(TEXTURE(ShadowMap), IN.PosLightSpace, light.a);

	float3 w = IN.Weights.xyz;

    float4 albedo = sampleBlend(TEXTURE(AlbedoMap), IN.Uv0, IN.Uv1, IN.Uv2, w);

#ifdef PIN_HQ
	float fade = saturate0(1 - IN.View_Depth.w * G(FadeDistance_GlowFactor).y);

#ifndef PIN_GBUFFER
	float3 normal = IN.Normal;
#else
	float3 normal = sampleNormal(TEXTURE(NormalMap), IN.Uv0, IN.Uv1, IN.Uv2, w, IN.Normal, IN.Tangents);
#endif

	float4 params = sampleBlend(TEXTURE(SpecularMap), IN.Uv0, IN.Uv1, IN.Uv2, w);

    float ndotl = dot(normal, -G(Lamp0Dir));

    // Compute diffuse term
    float3 diffuse = (G(AmbientColor) + (saturate(ndotl) * G(Lamp0Color) + max(-ndotl, 0) * G(Lamp1Color)) * shadow + light.rgb + params.b * 2) * albedo.rgb;

	// Compute specular term
    float specularIntensity = step(0, ndotl) * params.r * fade;
    float specularPower = params.g * 128 + 0.01;

	float3 specular = G(Lamp0Color) * (specularIntensity * shadow * (float)(half)pow(saturate(dot(normal, normalize(-G(Lamp0Dir) + normalize(IN.View_Depth.xyz)))), specularPower));
#else
    // Compute diffuse term
    float3 diffuse = (G(AmbientColor) + IN.Diffuse * shadow + light.rgb) * albedo.rgb;

	// Compute specular term
	float3 specular = 0;
#endif

    // Combine
    oColor0.rgb = diffuse + specular;
    oColor0.a = 1;

    float fogAlpha = saturate(IN.LightPosition_Fog.w);

    oColor0.rgb = lerp(G(FogColor), oColor0.rgb, fogAlpha);

#ifdef PIN_GBUFFER
    oColor1 = gbufferPack(IN.View_Depth.w, diffuse.rgb, specular.rgb, fogAlpha);
#endif
}
