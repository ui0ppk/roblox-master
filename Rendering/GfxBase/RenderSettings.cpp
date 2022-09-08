#include "GfxBase/RenderSettings.h"

#include "rbx/Debug.h"

namespace RBX {

const G3D::Vector2int16 CRenderSettings::minGameWindowSize = G3D::Vector2int16((G3D::int16)816,(G3D::int16)638);

CRenderSettings::GraphicsMode CRenderSettings::latchedGraphicsMode = CRenderSettings::UnknownGraphicsMode;

CRenderSettings::AASamples CRenderSettings::aaSamples(defaultAASamples);

const CRenderSettings::RESOLUTIONENTRY ResolutionTable [] = {
	{ CRenderSettings::Resolution720x526,	720,526 },
	{ CRenderSettings::Resolution800x600,	800,600 },
	{ CRenderSettings::Resolution1024x600,	1024,600 },
	{ CRenderSettings::Resolution1024x768,	1024,768 },
	{ CRenderSettings::Resolution1280x720,	1280,720 },
	{ CRenderSettings::Resolution1280x768,	1280,768 },
	{ CRenderSettings::Resolution1152x864,	1152,864 },
	{ CRenderSettings::Resolution1280x800,	1280,800 },
	{ CRenderSettings::Resolution1360x768,	1360,768 },
	{ CRenderSettings::Resolution1280x960,	1280,960 },
	{ CRenderSettings::Resolution1280x1024,	1280,1024 },
	{ CRenderSettings::Resolution1440x900,	1440,900 },
	{ CRenderSettings::Resolution1600x900,	1600,900 },
	{ CRenderSettings::Resolution1600x1024,	1600,1024 },
	{ CRenderSettings::Resolution1600x1200,	1600,1200 },
	{ CRenderSettings::Resolution1680x1050,	1680,1050 },
	{ CRenderSettings::Resolution1920x1080,	1920,1080 },
	{ CRenderSettings::Resolution1920x1200,	1920,1200 }
};

CRenderSettings::CRenderSettings()
#if !RBX_PLATFORM_IOS
	: fullscreenSize(G3D::Vector2int16(800,  600)) // these are just fail safes in case auto detect procedure fails.
	, windowSize(G3D::Vector2int16(800,  600))    //
#else
    : fullscreenSize(G3D::Vector2int16(1024,  768)) // these are just fail safes in case auto detect procedure fails.
	, windowSize(G3D::Vector2int16(1024, 768))    //
#endif
	, graphicsMode(AutoGraphicsMode)
	, qualityLevel(QualityAuto)
	, editQualityLevel(QualityAuto)
	, antialiasingMode(AntialiasingOff)
	, frameRateManagerMode(FrameRateManagerAuto)
	, showAggregation(false)
	, drawConnectors(false)
	, minCullDistance(50)
	, debugShowBoundingBoxes(false) // debug.
	, debugReloadAssets(false) // debug.
    , objExportMergeByMaterial(false)
	, eagerBulkExecution(false) // debug
	, enableFRM(true)
	, autoQualityLevel(1)
	, resolutionPreference(ResolutionAuto)
	, maxQualityLevel(QualityLevelMax)
	, textureCacheSize(1024 * 1024 * 32) // 32 MB
	, meshCacheSize(1024 * 1024 * 32) // 32 MB
{
}

const CRenderSettings::RESOLUTIONENTRY& CRenderSettings::getResolutionPreset(ResolutionPreset preset) const
{
	RBXASSERT(preset < ResolutionMaxIndex);
	RBXASSERT(ResolutionTable[preset-1].preset == preset);
	RBXASSERT(ResolutionMaxIndex == ARRAYSIZE(ResolutionTable)+1);

	return ResolutionTable[preset-1];
}

}  // namespace RBX
