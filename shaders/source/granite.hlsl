#define CFG_TEXTURE_TILING              1

#define CFG_DIFFUSE_SCALE               1
#define CFG_SPECULAR_SCALE              0.5
#define CFG_GLOSS_SCALE                 128
#define CFG_REFLECTION_SCALE          	0.2

#define CFG_NORMAL_SHADOW_SCALE         0.1

#define CFG_SPECULAR_LOD				0.19
#define CFG_GLOSS_LOD					24

#define CFG_NORMAL_DETAIL_TILING		0
#define CFG_NORMAL_DETAIL_SCALE			0

#define CFG_FAR_TILING					0.25
#define CFG_FAR_DIFFUSE_CUTOFF			0.6
#define CFG_FAR_NORMAL_CUTOFF			0
#define CFG_FAR_SPECULAR_CUTOFF			0

#define CFG_OPT_NORMAL_CONST

#include "material.hlsl"
