#include "common.h"

struct Appdata
{
    float4 Position     : POSITION;

    ATTR_INT4 Extra : COLOR1;
};

struct VertexOutput
{
	float4 HPosition: POSITION;

    float3 PosLightSpace: TEXCOORD0;
};

#ifdef PIN_SKINNED
    WORLD_MATRIX_ARRAY(WorldMatrixArray, MAX_BONE_COUNT * 3);
#endif

VertexOutput ShadowVS(Appdata IN)
{
    VertexOutput OUT = (VertexOutput)0;

    // Transform position to world space
#ifdef PIN_SKINNED
    int boneIndex = IN.Extra.r;

    float4 worldRow0 = WorldMatrixArray[boneIndex * 3 + 0];
    float4 worldRow1 = WorldMatrixArray[boneIndex * 3 + 1];
    float4 worldRow2 = WorldMatrixArray[boneIndex * 3 + 2];
		
	float3 posWorld = float3(dot(worldRow0, IN.Position), dot(worldRow1, IN.Position), dot(worldRow2, IN.Position));
#else
	float3 posWorld = IN.Position.xyz;
#endif

	OUT.HPosition = mul(G(ViewProjection), float4(posWorld, 1));

    OUT.PosLightSpace = shadowPrepareSample(posWorld);

	return OUT;
}

float4 ShadowPS(VertexOutput IN): COLOR0
{
	float depth = shadowDepth(IN.PosLightSpace);

	return float4(depth, 1, 0, 0);
}
