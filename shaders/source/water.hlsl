
//
// Water shader.
// Big, fat and ugly.
//
// All (most) things considered, I have converged to this particular way of rendering water:
//
//   Vertex waves
//   No transparency. Solid color for deep water.
//   Fresnel law, reflects environment.
//   Phong speculars.
//   Ripples via animated normal map. Adjustable intensity, speed and scale. Affect reflection and speculars.

#include "common.h"

WORLD_MATRIX(WorldMatrix);

uniform float4 nmAnimLerp; // ratio between normal map frames
uniform float4 waveParams; // .x = frequency  .y = phase  .z = height
uniform float4 WaterColor; // deep water color

#ifdef PIN_HQ
#	define WATER_LOD 1
#else
#	define WATER_LOD 2
#endif

#define LODBIAS (-1)

float  fadeFactor( float3 wspos )
{
	return saturate( -0.4f + 1.4f*length( G(CameraPosition) - wspos.xyz ) * G(FadeDistance_GlowFactor).y );
}

float  wave( float4 wspos )
{
	float x = sin( ( wspos.z - wspos.x - waveParams.y ) * waveParams.x );
	float z = sin( ( wspos.z + wspos.x + waveParams.y ) * waveParams.x );
	float p = (x + z) * waveParams.z;
	return  p - p * fadeFactor( wspos.xyz );
}



// perturbs the water mesh and vertex normals
void makeWaves( inout float4 wspos, inout float3 wsnrm )
{
#if WATER_LOD == 0 
	float gridSize = 4.0f;

	float4 wspos1 = wspos;
	float4 wspos2 = wspos;

	wspos1.x += gridSize;
	wspos2.z += gridSize;
	
	wspos.y  += wave(wspos) ;
	wspos1.y += wave(wspos1);
	wspos2.y += wave(wspos2);
	
	wsnrm = normalize( cross( wspos2.xyz - wspos.xyz, wspos1.xyz - wspos.xyz ) );
#elif WATER_LOD == 1
	wspos.y += wave( wspos );
#else   /* do n0thing */
#endif
}

struct V2P
{
	float4 pos    : POSITION;
	float4 tc0Fog : TEXCOORD0;
	float4 wspos  : TEXCOORD1;
	float3 wsnrm  : TEXCOORD2;
	float3 light  : TEXCOORD3;
	float3 fade   : TEXCOORD4;
};

V2P water_vs(
	ATTR_INT4 pos : POSITION,
	ATTR_INT3 nrm : NORMAL
)
{
	V2P o;

    // Decode vertex data
    float3 normal = (nrm - 127.0) / 127.0;

	normal = normalize(normal);
	
	float4 wspos = mul( WorldMatrix, pos );
	float3 wsnrm = normal;
	
	wspos.y -= 2*waveParams.z;
	
	makeWaves( /*INOUT*/ wspos, /*INOUT*/ wsnrm );
	
	o.wspos = wspos;
	o.wsnrm = wsnrm;
	
	if( normal.y < 0.01f ) o.wsnrm = normal;

	// box mapping
	//float3x2 m = { wspos.xz, wspos.xy, wspos.yz };
	//float2 tcselect = mul( abs( nrm.yzx ), m );

	float2 tcselect;
	float3 wspostc = float3( wspos.x, -wspos.y, wspos.z );

	tcselect.x = dot( abs( normal.yxz ), wspostc.xzx );
	tcselect.y = dot( abs( normal.yxz ), wspostc.zyy );

	o.pos       = mul( G(ViewProjection), wspos );
	o.tc0Fog.xy = tcselect * .05f;
	o.tc0Fog.z  = saturate( (G(FogParams).z - o.pos.w) * G(FogParams).w );
	o.tc0Fog.w  = LODBIAS;

	o.light = lgridPrepareSample(lgridOffset(wspos.xyz, wsnrm.xyz));
	
	o.fade.x = fadeFactor( wspos.xyz );
	o.fade.y = (1-o.fade.x) *  saturate( dot( wsnrm, -G(Lamp0Dir) ) ) * 100;
	o.fade.z =  1 - 0.9*saturate1( exp( -0.005 * length( G(CameraPosition) - wspos.xyz ) ) );
	
	return o;
}

//////////////////////////////////////////////////////////////////////////////

TEX_DECLARE2D(NormalMap1, 0);
TEX_DECLARE2D(NormalMap2, 1);
TEX_DECLARECUBE(EnvMap, 2);
LGRID_SAMPLER(LightMap, 3);
TEX_DECLARE2D(LightMapLookup, 4);

float3 pixelNormal( float4 tc0 )
{
	float4 nm1 = tex2Dbias( NormalMap1, tc0 );
#if WATER_LOD <= 1
	float4 nm2 = tex2Dbias( NormalMap2, tc0 );
	float4 nm3 = lerp( nm1, nm2, nmAnimLerp.xxxx );
#else
	float4 nm3 = nm1;
#endif
	return nmapUnpack( nm3 );
}

// Fresnel approximation. N1 and N2 are refractive indices.
// for above water, use n1 = 1, n2 = 1.3, for underwater use n1 = 1.3, n2 = 1
float fresnel( float3 N, float3 V, float n1, float n2, float p, float fade )
{
#if WATER_LOD == 0
	float r0 = (n1-n2)/(n1+n2);
	r0 *= r0;
	return r0 + (1-r0) * pow( 1 - abs( dot( normalize(N), V ) ), p );
#else
	return 0.1 + saturate( - 1.9 * abs( dot( N, V ) ) + 0.8); // HAXX!
	//return 1 - 2 * abs( dot( N, V ) );
#endif
}

float4 envColor( float3 N, float3 V, float fade )
{
	float3 dir = reflect( V, N );
    return texCUBE(EnvMap, dir) * 0.91f;
}

float4 deepWaterColor(float4 light)
{
	//float4 tint = 5*float4( 0.1f, 0.1f, 0.13f, 1);
	float4 tint = 0.8f*float4( 118, 143, 153, 255 ) / 255;
	return (light + texCUBEbias( EnvMap, float4( 0,1,0, 10.0f) )) * tint;
}


//////////////////////////////////////////
//////////////////////////////////////////


 
float4 water_ps( V2P v ) : COLOR0
{

	float4 WaterColorTest = 0.5 * float4(  26, 169, 185, 0  ) / 255;
	float4 FogColorTest = 0.8 * float4( 35, 107, 130, 0 ) / 255;
	
	float3 N2 = v.wsnrm;
	float3 N1 = pixelNormal( v.tc0Fog ).xzy;
	float3 N3 = 0.5*(N2 +  N1);
	
	N3 = lerp( N3, N2, v.fade.z );

	float3 L = /*normalize*/(-G(Lamp0Dir).xyz);
	float3 E = normalize( G(CameraPosition) - v.wspos.xyz );

	float4 light = lgridSample(TEXTURE(LightMap), TEXTURE(LightMapLookup), v.light.xyz);
	
	float  fre     =  fresnel( N3, E, 1.0f, 1.3f, 5, v.fade.x );
	float3 diffuse =  deepWaterColor(light).rgb;
	float3 env     =  envColor( N3, -E, v.fade.x ).rgb;

	float3 R = reflect( -L, N1 );  
	
#if WATER_LOD <= 1
	float specular = pow( saturate0( dot( R, E ) ), 1600 ) * L.y * 100; // baseline
#	ifndef GLSLES
		specular = 0.65 * saturate1( specular * saturate0( light.a - 0.4f ) );
#	endif
#else
	float specular = 0;
#endif

	float3 result = lerp( diffuse, env, fre ) + specular.xxx;
	result = lerp( G(FogColor).rgb, result, v.tc0Fog.z );
		
	return float4( result, 1 );
}
