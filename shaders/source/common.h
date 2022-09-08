#include "globals.h"

// GLSLES has limited number of vertex shader registers so we have to use less bones
#if defined(GLSLES) && !defined(GL3)
#define MAX_BONE_COUNT 32
#else
#define MAX_BONE_COUNT 72
#endif

// PowerVR saturate() is compiled to min/max pair
// These are cross-platform specialized saturates that are free on PC and only cost 1 cycle on PowerVR
#ifdef GLSLES
float saturate0(float v) { return max(v, 0); }
float saturate1(float v) { return min(v, 1); }
#define WANG_SUBSET_SCALE 2
#else
float saturate0(float v) { return saturate(v); }
float saturate1(float v) { return saturate(v); }
#define WANG_SUBSET_SCALE 1
#endif

#define GBUFFER_MAX_DEPTH 500.0f 


#ifndef DX11
    #define TEX_DECLARE2D(name, reg) sampler2D name: register(s##reg)
    #define TEX_DECLARE3D(name, reg) sampler3D name: register(s##reg)
    #define TEX_DECLARECUBE(name, reg) samplerCUBE name: register(s##reg)

    #define TEXTURE(name) name
    #define TEXTURE_IN_2D(name) sampler2D name
    #define TEXTURE_IN_3D(name) sampler3D name
    #define TEXTURE_IN_CUBE(name) samplerCUBE name

    #define WORLD_MATRIX(name) uniform float4x4 name;
    #define WORLD_MATRIX_ARRAY(name, count) uniform float4 name [ count ];

	#ifdef GLSL
		#define ATTR_INT4 float4
		#define ATTR_INT3 float3
		#define ATTR_INT2 float2
		#define ATTR_INT float
	#else
		#define ATTR_INT4 int4
		#define ATTR_INT3 int3
		#define ATTR_INT2 int2
		#define ATTR_INT int
	#endif
#else
    #define TEX_DECLARE2D(name, reg) SamplerState name##Sampler: register(s##reg); Texture2D<float4> name##Texture: register(t##reg)
    #define TEX_DECLARE3D(name, reg) SamplerState name##Sampler: register(s##reg); Texture3D<float4> name##Texture: register(t##reg)
    #define TEX_DECLARECUBE(name, reg) SamplerState name##Sampler: register(s##reg); TextureCube<float4> name##Texture: register(t##reg)

    #define tex2D(tex, uv) tex##Texture.Sample(tex##Sampler, uv)
    #define tex3D(tex, uv) tex##Texture.Sample(tex##Sampler, uv)
    #define texCUBE(tex, uv) tex##Texture.Sample(tex##Sampler, uv)
    #define tex2Dgrad(tex, uv, DDX, DDY) tex##Texture.SampleGrad(tex##Sampler, uv, DDX, DDY)
    #define tex2Dbias(tex, uv) tex##Texture.SampleBias(tex##Sampler, uv.xy, uv.w)
    #define texCUBEbias(tex, uv) tex##Texture.SampleBias(tex##Sampler, uv.xyz, uv.w)

    #define TEXTURE(name) name##Sampler, name##Texture
    #define TEXTURE_IN_2D(name) SamplerState name##Sampler, Texture2D name##Texture
    #define TEXTURE_IN_3D(name) SamplerState name##Sampler, Texture3D name##Texture
    #define TEXTURE_IN_CUBE(name) SamplerState name##Sampler, TextureCube name##Texture

    #define WORLD_MATRIX(name) cbuffer WorldMatrixCB : register( b1 ) { float4x4 name; }
    #define WORLD_MATRIX_ARRAY(name, count) cbuffer WorldMatrixCB : register( b1 ) { float4 name[ count ]; }

    #define ATTR_INT4 int4
    #define ATTR_INT3 int3
    #define ATTR_INT2 int2
	#define ATTR_INT int
#endif

#if defined(GLSLES) || defined(PIN_WANG_FALLBACK)
    #define TEXTURE_WANG(name) 0
    void getWang(float unused, float2 uv, float tiling, out float2 wangUv, out float4 wangUVDerivatives)
    {
        wangUv = uv * WANG_SUBSET_SCALE;
        wangUVDerivatives = float4(0,0,0,0);    // not used in this mode 
    }
    float4 sampleWang(TEXTURE_IN_2D(s), float2 uv, float4 wangUVDerivatives)
    {
        return tex2D(s,uv);
    }
#else
    #define TEXTURE_WANG(name) TEXTURE(name)
    void getWang(TEXTURE_IN_2D(s), float2 uv, float tiling, out float2 wangUv, out float4 wangUVDerivatives)
    {
    #ifndef WIN_MOBILE
        float idxTexSize = 128;
    #else
        float idxTexSize = 32;
    #endif

        float2 wangBase = uv * tiling * 4;
        
    #if defined(DX11) && !defined(WIN_MOBILE)
        // compensate the precision problem of Point Sampling on some cards. (We do it just at DX11 for performance reasons)
        float2 wangUV = (floor(wangBase) + 0.5) / idxTexSize;
    #else
        float2 wangUV = wangBase / idxTexSize;
    #endif

    #if defined(DX11) || defined(GL3)
        float2 wang = tex2D(s, wangUV).rg;
    #else
        float2 wang = tex2D(s, wangUV).ba;
    #endif

        wangUVDerivatives = float4(ddx(wangBase*0.25), ddy(wangBase*0.25));

        wang *= 255.0/256.0;
        wangUv = wang + frac(wangBase)*0.25;
    }
    float4 sampleWang(TEXTURE_IN_2D(s), float2 uv, float4 derivates)
    {
        return tex2Dgrad(s, uv, derivates.xy, derivates.zw);
    }
#endif

float4 gbufferPack(float depth, float3 diffuse, float3 specular, float fog)
{
	depth = saturate(depth / GBUFFER_MAX_DEPTH);
	
	const float3 bitSh	= float3(255*255, 255, 1);
    const float3 lumVec = float3(0.299, 0.587, 0.114);

	float2 comp;
	comp = depth*float2(255,255*256);
	comp = frac(comp);
	comp = float2(depth,comp.x*256/255) - float2(comp.x, comp.y)/255;
	
	float4 result;
	
	result.r = lerp(1, dot(specular, lumVec), saturate(3 * fog));
	result.g = lerp(0, dot(diffuse, lumVec), saturate(3 * fog));
	result.ba = comp.yx;
	
	return result;
}

float3 lgridOffset(float3 v, float3 n)
{
    // cells are 4 studs in size
    // offset in normal direction to prevent self-occlusion
    // the offset has to be 1.5 cells in order to fully eliminate the influence of the source cell with trilinear filtering
    // (i.e. 1 cell is enough for point filtering, but is not enough for trilinear filtering)
    return v + n * (1.5f * 4.f);
}

float3 lgridPrepareSample(float3 c)
{
    // yxz swizzle is necessary for GLSLES sampling to work efficiently
    // (having .y as the first component allows to do the LUT lookup as a non-dependent texture fetch)
    return c.yxz * G(LightConfig0).xyz + G(LightConfig1).xyz;
}

#if defined(GLSLES) && !defined(GL3)
#define LGRID_SAMPLER(name, register) TEX_DECLARE2D(name, register)

float4 lgridSample(TEXTURE_IN_2D(t), TEXTURE_IN_2D(lut), float3 data)
{
    float4 offsets = tex2D(lut, data.xy);

    // texture is 64 pixels high
    // let's compute slice lerp coeff
    float slicef = frac(data.x * 64);

    // texture has 64 slices with 8x8 atlas setup
    float2 base = saturate(data.yz) * 0.125;

    float4 s0 = tex2D(t, base + offsets.xy);
    float4 s1 = tex2D(t, base + offsets.zw);

    return lerp(s0, s1, slicef);
}
#else
#define LGRID_SAMPLER(name, register) TEX_DECLARE3D(name, register)

float4 lgridSample(TEXTURE_IN_3D(t), TEXTURE_IN_2D(lut), float3 data)
{
    float3 edge = step(G(LightConfig3).xyz, abs(data - G(LightConfig2).xyz));
    float edgef = saturate1(dot(edge, 1));

    // replace data with 0 on edges to minimize texture cache misses
    float4 light = tex3D(t, data.yzx - data.yzx * edgef);

    return lerp(light, G(LightBorder), edgef);
}
#endif

#ifdef GLSLES
float3 nmapUnpack(float4 value)
{
    return value.rgb * 2 - 1;
}
#else
float3 nmapUnpack(float4 value)
{
    float2 xy = value.ag * 2 - 1;

    return float3(xy, sqrt(saturate(1 + dot(-xy, xy))));
}
#endif

float3 terrainNormal(float4 tnp0, float4 tnp1, float4 tnp2, float3 w, float3 normal, float3 tsel)
{
	// Inspired by "Voxel-Based Terrain for Real-Time Virtual Simulations" [Lengyel2010] 5.5.2
	float3 tangentTop = float3(normal.y, -normal.x, 0);
	float3 tangentSide = float3(normal.z, 0, -normal.x);

	float3 bitangentTop = float3(0, -normal.z, normal.y);
	float3 bitangentSide = float3(0, -1, 0);

	// Blend pre-unpack to save cycles
	float3 tn = nmapUnpack(tnp0 * w.x + tnp1 * w.y + tnp2 * w.z);

	// We blend all tangent frames together as a faster approximation to the correct world normal blend
	float tselw = dot(tsel, w);

	float3 tangent = lerp(tangentSide, tangentTop, tselw);
	float3 bitangent = lerp(bitangentSide, bitangentTop, tselw);

	return normalize(tangent * tn.x + bitangent * tn.y + normal * tn.z);
}

float3 shadowPrepareSample(float3 p)
{
	float4 c = float4(p, 1);

	return float3(dot(G(ShadowMatrix0), c), dot(G(ShadowMatrix1), c), dot(G(ShadowMatrix2), c));
}

float shadowDepth(float3 lpos)
{
	return lpos.z;
}

float shadowStep(float d, float z)
{
	// saturate returns 1 for z in [0.1..0.9]; it fades to 0 as z approaches 0 or 1
	return step(d, z) * saturate(9 - 20 * abs(z - 0.5));
}

float shadowSample(TEXTURE_IN_2D(map), float3 lpos, float lightShadow)
{
    float2 smDepth = tex2D(map, lpos.xy).rg;
	float smShadow = shadowStep(smDepth.x, shadowDepth(lpos));

	return (1 - smShadow * smDepth.y * G(OutlineBrightness_ShadowInfo).w) * lightShadow;
}
