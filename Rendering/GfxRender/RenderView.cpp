#include "stdafx.h"
#include "RenderView.h"

#include "Util/FileSystem.h"
#include "g3d/GImage.h"
#include "g3d/BinaryOutput.h"
#include "V8DataModel/Lighting.h"
#include "V8DataModel/Decal.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/Sky.h"
#include "V8DataModel/ContentProvider.h"
#include "V8DataModel/MeshContentProvider.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/TextService.h"
#include "V8DataModel/RenderHooksService.h"
#include "V8DataModel/Stats.h"
#include "v8datamodel/DataModel.h"
#if defined(RBX_PLATFORM_DURANGO)
#   include "v8datamodel/PlatformService.h"
#endif
#include "v8world/World.h"
#include "v8datamodel/UserInputService.h"

#include "util/standardout.h"
#include "rbx/MathUtil.h"
#include "VisualEngine.h"
#include "VertexStreamer.h"
#include "TypesetterBitmap.h"
#include "Water.h"

#include "SceneManager.h"
#include "SceneUpdater.h"

#include "TextureCompositor.h"

#include "GfxBase/RenderStats.h"
#include "GfxBase/FrameRateManager.h"
#include "util/Profiling.h"
#include "util/IMetric.h"
#include "util/RobloxGoogleAnalytics.h"

#include "TextureManager.h"
#include "AdornRender.h"
#include "LightGrid.h"
#include "MaterialGenerator.h"
#include "GeometryGenerator.h"

#if !defined(RBX_PLATFORM_DURANGO)
#include "ObjectExporter.h"
#endif
#include "TextureAtlas.h"

#include "FastLog.h"

#include "Sky.h"
#include "Util.h"
#include "rbx/SystemUtil.h"

#include "GfxCore/Framebuffer.h"
#include "GfxCore/Device.h"

#include "SpatialHashedScene.h"
#include "CullableSceneNode.h"
#include "LightObject.h"

#include "rbx/rbxTime.h"

#include "rbx/Profiler.h"
#include "GfxCore/States.h"
#include "ShaderManager.h"

#include "G3D/Quat.h"

#include "GfxBase/AdornBillboarder.h"

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
#include <mmsystem.h>
#endif

LOGGROUP(ThumbnailRender)
LOGGROUP(RenderBreakdown)
LOGGROUP(RenderStatsOnLogs)
LOGGROUP(Graphics)

FASTFLAGVARIABLE(DebugAdornsDisabled, false)

DYNAMIC_FASTFLAGVARIABLE(ScreenShotDuplicationFix, false)

FASTINTVARIABLE(OutlineBrightnessMin, 50)
FASTINTVARIABLE(OutlineBrightnessMax, 160)
FASTINTVARIABLE(OutlineThickness, 40)
FASTFLAGVARIABLE(RenderThumbModelReflectionsFix,false);

FASTINTVARIABLE(RenderShadowIntensity, 75)

FASTFLAG(UseDynamicTypesetterUTF8)
FASTFLAG(FramerateVisualizerShow)
FASTFLAG(TaskSchedulerCyclicExecutive)

// Test: 0 - no throttling, 1 - adds 5ms to frame time, 2 - adds 10ms to frame time
ABTEST_NEWUSERS_VARIABLE(FrameRateThrottling)
FASTFLAG(ClientABTestingEnabled)

FASTFLAG(TypesettersReleaseResources);

FASTFLAGVARIABLE(RenderFixFog, false)

FASTFLAGVARIABLE(RenderVR, false)

FASTFLAGVARIABLE(RenderLowLatencyLoop, false)

FASTFLAGVARIABLE(RenderUIAs3DInVR, true)

// Studs-to-meters ratio
const float kVRScale = 3.f;

// Canvas size for 2D UI in VR
const float kVRUICanvas = 720;

// Depth of 2D UI in studs
const float kVRUIDepth = 3;

// Field of view of 2D UI in degrees
const float kVRUIFOV = 50;

#ifdef _WIN32
extern "C"
{
	_declspec(dllexport) int NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif


namespace RBX
{

void RenderView_InitModule()
{
	class RenderViewFactory : public IViewBaseFactory
	{
	public:
		RenderViewFactory()
		{
			ViewBase::RegisterFactory(CRenderSettings::Direct3D9, this);
            ViewBase::RegisterFactory(CRenderSettings::Direct3D11, this);
			ViewBase::RegisterFactory(CRenderSettings::OpenGL, this);
		}

		ViewBase* Create(CRenderSettings::GraphicsMode mode, OSContext* context, CRenderSettings* renderSettings)
		{
			return new Graphics::RenderView(mode, context, renderSettings);
		}
	};

	static RenderViewFactory factory;
}

void RenderView_ShutdownModule()
{
}

}

namespace RBX
{
namespace Graphics
{
double busyWaitLoop(double ms);

class FrameRateManagerStatsItem : public RBX::Stats::Item
{
	int qualityLevel;
	bool autoQuality;
	int numberOfSettles;
	double averageSwitches;
    double averageFPS;

public:
	FrameRateManagerStatsItem()
		: qualityLevel(0)
        , autoQuality(false)
        , numberOfSettles(0)
		, averageSwitches(0)
        , averageFPS(0)
	{
	}

	static shared_ptr<FrameRateManagerStatsItem> create()
	{
		shared_ptr<FrameRateManagerStatsItem> result = RBX::Creatable<RBX::Instance>::create<FrameRateManagerStatsItem>();

		result->createBoundChildItem("QualityLevel", result->qualityLevel);
		result->createBoundChildItem("AutoQuality", result->autoQuality);
		result->createBoundChildItem("NumberOfSettles", result->numberOfSettles);
		result->createBoundChildItem("AverageSwitches", result->averageSwitches);
        result->createBoundChildItem("AverageFPS", result->averageFPS);

		return result;
	}

	void updateData(FrameRateManager* frm)
	{
		RBX::FrameRateManager::Metrics metrics = frm->GetMetrics();

		qualityLevel = metrics.QualityLevel;
		autoQuality = metrics.AutoQuality;
		numberOfSettles = metrics.NumberOfSettles;
		averageSwitches = metrics.AverageSwitchesPerSettle;
        averageFPS = metrics.AverageFps;
	}
};

struct GraphicsModeTranslation
{
    CRenderSettings::GraphicsMode settingsItem;
    Device::API api;
};

static const unsigned validGraphicsModes = 3;
static const GraphicsModeTranslation gGraphicsModesTranslation[validGraphicsModes] =
{
    { CRenderSettings::Direct3D9, Device::API_Direct3D9 },
    { CRenderSettings::Direct3D11, Device::API_Direct3D11},
    { CRenderSettings::OpenGL, Device::API_OpenGL}
};

static CoordinateFrame computeVRFrame(const DeviceVR::Pose& pose)
{
	CoordinateFrame result;
	result.translation = Vector3(pose.position) * kVRScale;
	result.rotation = G3D::Quat(pose.orientation).toRotationMatrix();

	return result;
}

static Vector2 computeCanvasSize(Device* device)
{
	if (device->getVR())
	{
		return Vector2(kVRUICanvas, kVRUICanvas);
	}
	else if (Framebuffer* framebuffer = device->getMainFramebuffer())
	{
		int viewScale = (device->getCaps().retina) ? 2 : 1;

		return Vector2(framebuffer->getWidth(), framebuffer->getHeight()) / viewScale;
	}
	else
	{
		return Vector2();
	}
}

RenderView::RenderView(CRenderSettings::GraphicsMode graphicsMode, OSContext* context, CRenderSettings* renderSettings) 
	: deltaMs(0)
	, timestampMs(0)
    , prepareTime(0)
    , totalRenderTime(0)
	, prepareAverage(15)
	, performAverage(15)
	, presentAverage(15)
    , artificialDelay(0)
	, gpuAverage(15)
	, lightingValid(false)
	, doScreenshot(false)
	, adornsEnabled(true)
	, outlinesEnabled(true)
	, fogEndCurrentFRM(10000)
	, fogEndTargetFRM(10000)
    , frameDataCallback(FrameDataCallback())
{
	FASTLOG1(FLog::ViewRbxInit, "RenderView created - %p", this);

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
	timeBeginPeriod(1);
#endif

    bool translatioFound = false;
    for (unsigned i = 0; i < validGraphicsModes; ++i)
    {
        if (gGraphicsModesTranslation[i].settingsItem == graphicsMode)
        {
            device.reset(Device::create(gGraphicsModesTranslation[i].api, context->hWnd));
            translatioFound = true;
            break;
        }
    }

    if (!translatioFound)
        throw RBX::runtime_error("Not supported graphics mode");
    
    const DeviceCaps& caps = device->getCaps();

	if (!caps.supportsFramebuffer)
		throw std::runtime_error("Device does not support framebuffers");

	if (!caps.supportsShaders && !caps.supportsFFP)
		throw std::runtime_error("Device does not support shaders or FFP");

	visualEngine.reset(new VisualEngine(device.get(), renderSettings));
}

void RenderView::enableAdorns(bool enable)
{
	adornsEnabled = enable;
}

void RenderView::initResources()
{
	FASTLOG1(FLog::ViewRbxInit, "RenderView::initResources - %p", this);

	FASTLOG(FLog::ViewRbxInit, "RenderView::initResources finish");
}

void RenderView::bindWorkspace(boost::shared_ptr<RBX::DataModel> dataModel)
{
	if (this->dataModel == dataModel)
		return;

	FASTLOG2(FLog::ViewRbxInit, "BindWorkspace, new datamodel %p, old datamodel: %p", dataModel.get(), this->dataModel.get());

	this->lightingValid = false;
	this->doScreenshot = false;

	if (this->dataModel)
	{
        if (frameRateManagerStatsItem)
        {
            frameRateManagerStatsItem->setParent(NULL);
            frameRateManagerStatsItem.reset();
        }

		lightingChangedConnection.disconnect();
		screenshotConnection.disconnect();

		if(TextService* textService = ServiceProvider::create<TextService>(this->dataModel.get()))
		{
			textService->clearTypesetters();
		}

        visualEngine->bindWorkspace(shared_ptr<DataModel>());
	}

	this->dataModel = dataModel;
  
	if (this->dataModel)
	{
        visualEngine->bindWorkspace(dataModel);

		Lighting* lighting = ServiceProvider::create<Lighting>(dataModel.get());
		lightingChangedConnection = lighting->lightingChangedSignal.connect(boost::bind(&RenderView::invalidateLighting, this, _1));
		screenshotConnection = this->dataModel->screenshotSignal.connect(boost::bind(&RenderView::onTakeScreenshot, this));

		if (TextService* textService = ServiceProvider::create<TextService>(dataModel.get()))
		{
			for (Text::Font font = Text::FONT_LEGACY; font != Text::FONT_LAST; font=Text::Font(font+1))
			{
				if (FFlag::TypesettersReleaseResources)
				{
					visualEngine->getTypesetter(font)->loadResources(visualEngine->getTextureManager(), visualEngine->getGlyphAtlas());
				}
				textService->registerTypesetter(TextService::FromTextFont(font), visualEngine->getTypesetter(font));	
			}
		}

		FrameRateManager* frm = visualEngine->getFrameRateManager();
		if (frm)
        {
			frm->StartCapturingMetrics();

            RBX::Stats::StatsService* stats = RBX::ServiceProvider::create<RBX::Stats::StatsService>(this->dataModel.get());

            frameRateManagerStatsItem = FrameRateManagerStatsItem::create();
            frameRateManagerStatsItem->setName("FrameRateManager");
            frameRateManagerStatsItem->setParent(stats);
			frameRateManagerStatsItem->updateData(frm);
        }
        
        if (visualEngine->getDevice()->getMainFramebuffer())
        {
            this->dataModel->setInitialScreenSize(computeCanvasSize(visualEngine->getDevice()));
        }

		RBX::RenderHooksService* service = RBX::ServiceProvider::find<RBX::RenderHooksService>(dataModel.get());
		service->setRenderHooks(this);

		service->reloadShadersSignal.connect(boost::bind(&RenderView::reloadShaders, this));

		// default fog
		fogEndCurrentFRM = fogEndTargetFRM = lighting->getFogEnd();

        visualEngine->getSettings()->setDrawConnectors(dataModel->isStudio());
	}
}


RenderView::~RenderView(void)
{	
	bindWorkspace(boost::shared_ptr<RBX::DataModel>());

    sendFeatureLevelStats();

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
	timeEndPeriod(1);
#endif
	FASTLOG(FLog::ViewRbxInit, "RenderView destroyed");
}

void RenderView::sendFeatureLevelStats()
{
    if (visualEngine.get() && visualEngine.get()->getDevice())
    {
        std::string osAndFeatureLvl = SystemUtil::osPlatform() + " " + visualEngine.get()->getDevice()->getFeatureLevel();
        RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "GraphicsFeatureLevel", osAndFeatureLvl.c_str());
    }
}

void RenderView::onResize(int cx, int cy)
{
}

FrameRateManager* RenderView::getFrameRateManager()
{
	return visualEngine->getFrameRateManager();
}

RBX::Instance* RenderView::getWorkspace() 
{
	return dataModel->getWorkspace(); 
}	

void RenderView::invalidateLighting(bool updateSkybox)
{
	lightingValid = false;
}

void RenderView::onTakeScreenshot()
{
	doScreenshot = true;
}

bool RenderView::getAndClearDoScreenshot()
{
	bool result = doScreenshot;
	doScreenshot = false;
	return result;
}

void RenderView::updateFog()
{
	static const float TAU = 0.04f; // dampening factor as in:   exp( -x * TAU )

	RBX::Lighting* lighting = ServiceProvider::create<RBX::Lighting>(dataModel.get());

	if (FFlag::RenderFixFog)
	{
		float currDeltaFRM = fogEndTargetFRM - fogEndCurrentFRM;
		if (currDeltaFRM > 0)
			fogEndCurrentFRM += currDeltaFRM * TAU;
		else
			fogEndCurrentFRM = fogEndTargetFRM;
	   
		float fogEnd   = std::max(fogEndCurrentFRM, lighting->getFogEnd()*0.5f);  
		float fogStart = G3D::clamp(lighting->getFogStart(), 0.0f, fogEnd - 0.1f);

		visualEngine->getSceneManager()->setFog(lighting->getFogColor(), fogStart, fogEnd);
	}
	else
	{
		float frmEnd = this->fogEndTargetFRM;
		float fogStart = lighting->getFogStart();
		float fogEnd   = lighting->getFogEnd();
		float range = fogEnd - fogStart;

		// set fog
		fogEnd = std::min(fogEnd, frmEnd);
		float currDelta = fogEnd - this->fogEndCurrentFRM;
		fogEnd = this->fogEndCurrentFRM += currDelta * TAU;
		if (frmEnd < lighting->getFogEnd() && currDelta < 0)
		{
			fogEnd = this->fogEndCurrentFRM = frmEnd; // force instant transition
		}

		fogEnd = this->fogEndCurrentFRM = std::max(fogEnd, lighting->getFogEnd()*0.5f);
		fogStart = fogEnd - range * (fogEnd / lighting->getFogEnd());

		// Make sure fog range is not empty to avoid issues
		fogStart = G3D::clamp(fogStart, 0.0f, fogEnd - 0.1f);

		visualEngine->getSceneManager()->setFog(lighting->getFogColor(), fogStart, fogEnd);
	}
}

void RenderView::updateLighting(Lighting* lighting)
{
    SceneManager* smgr = visualEngine->getSceneManager();

	if (!lightingValid)
	{
        // Sky
        if (FFlag::RenderThumbModelReflectionsFix || !lighting->isSkySuppressed()) // prepare the sky regardless of whether it's suppressed, because it's needed for the envmaps
        {
			Sky* sky = smgr->getSky();

            if (lighting->sky)
			{
                sky->setSkyBox(lighting->sky->skyRt, lighting->sky->skyLf, lighting->sky->skyBk, lighting->sky->skyFt, lighting->sky->skyUp, lighting->sky->skyDn);
				sky->setCelestialBodies(lighting->sky->sunTexture, lighting->sky->moonTexture);
				sky->update(lighting->getSkyParameters(), lighting->sky->getNumStars(), lighting->sky->drawCelestialBodies, lighting->sky->sunAngularSize, lighting->sky->moonAngularSize);
			}
            else
			{
                sky->setSkyBoxDefault();
				sky->setCelestialBodiesDefault();
				sky->update(lighting->getSkyParameters(), 3000, true, 21, 11);
			}

            // Update clear color for time-of-day
            lighting->setClearColor(lighting->getSkyParameters().skyAmbient);
        }

        // Lighting
        presetLighting(lighting);

        lightingValid = true;
	}

    smgr->setSkyEnabled(!lighting->isSkySuppressed());
    smgr->setClearColor(Color4(lighting->getClearColor(), lighting->getClearAlpha()));
}

static void reportVRUsage(int placeId)
{
	std::string placeIdStr = format("%d", placeId);

	RobloxGoogleAnalytics::trackEventWithoutThrottling(GA_CATEGORY_GAME, "VR", placeIdStr.c_str());
}

void RenderView::updateVR()
{
	if (!FFlag::RenderVR)
		return;

	RBXPROFILER_SCOPE("Render", "updateVR");

    if (DeviceVR* vr = visualEngine->getDevice()->getVR())
	{
		// GA
		if (dataModel && dataModel->getPlaceID())
		{
			static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(flag, boost::bind(&reportVRUsage, dataModel->getPlaceID()));
		}

		// Disable throttling for VR (ideally render job should control this...)
		TaskScheduler::singleton().DataModel30fpsThrottle = false;

		// Put FRM into aggressive mode
		visualEngine->getFrameRateManager()->setAggressivePerformance(true);

		// Read VR sensor data
		vr->update();

		DeviceVR::State vrState = vr->getState();

		// Setup interaction between DataModel and game
		if (UserInputService* uis = ServiceProvider::find<UserInputService>(dataModel.get()))
		{
            RBXPROFILER_SCOPE("Render", "DM");

			uis->setVREnabled(true);

			if (vrState.headPose.valid)
			{
				CoordinateFrame cframe = computeVRFrame(vrState.headPose);

				// Legacy, to be removed
				uis->setUserHeadCFrame(cframe);

				uis->setUserCFrame(UserInputService::USERCFRAME_HEAD, cframe);
			}

			if (vrState.handPose[0].valid)
			{
				CoordinateFrame cframe = computeVRFrame(vrState.handPose[0]);

				uis->setUserCFrame(UserInputService::USERCFRAME_LEFTHAND, cframe);
			}

			if (vrState.handPose[1].valid)
			{
				CoordinateFrame cframe = computeVRFrame(vrState.handPose[1]);

				uis->setUserCFrame(UserInputService::USERCFRAME_RIGHTHAND, cframe);
			}

			if (uis->checkAndClearRecenterUserHeadCFrameRequest())
				vr->recenter();
		}
	}
	else
	{
		// Reenable throttling if necessary
		TaskScheduler::singleton().DataModel30fpsThrottle = DataModel::throttleAt30Fps;

		// Put FRM into non-aggressive mode
		visualEngine->getFrameRateManager()->setAggressivePerformance(false);

		// Clean up VR state
		if (UserInputService* uis = ServiceProvider::find<UserInputService>(dataModel.get()))
		{
			uis->setVREnabled(false);
			uis->setUserHeadCFrame(CoordinateFrame());
		}
	}
}

void RenderView::enableVR(bool enabled)
{
	if (!FFlag::RenderVR)
		return;

	visualEngine->getDevice()->setVR(enabled);
}

const char* RenderView::getVRDeviceName()
{
	return device->getVR() ? "VR" : 0;
}

double RenderView::getMetricValue(const std::string& name)
{
	FrameRateManager* frm = visualEngine->getFrameRateManager();

	if (name == "Delta Between Renders")
	{
		return frm != NULL ? frm->GetFrameTimeAverage() : 0.0;
	}  
	else if (name == "Total Render")
	{
		return frm != NULL ? frm->GetRenderTimeAverage() : 0.0;
	}
	else if (name == "Present Time")
	{
		return presentAverage.getStats().average;
	}
	else if (name == "GPU Delay")
	{
		return gpuAverage.getStats().average;
	}

	return -1.0;
}

std::string RenderView::getRenderStatsMetric(const std::string& name)
{
	if (name.compare(0, 15, "RenderStatsPass") == 0)
	{
		RenderPassStats* stats =
			(name == "RenderStatsPassScene") ? &visualEngine->getRenderStats()->passScene :
			(name == "RenderStatsPassShadow") ? &visualEngine->getRenderStats()->passShadow :
			(name == "RenderStatsPassUI") ? &visualEngine->getRenderStats()->passUI :
			(name == "RenderStatsPass3dAdorns") ? &visualEngine->getRenderStats()->pass3DAdorns :
			(name == "RenderStatsPassTotal") ? &visualEngine->getRenderStats()->passTotal :
			NULL;
			
		if (!stats) return "unknown pass";
			
		return RBX::format("%d (%df %dv %dp %ds)",
			stats->batches, stats->faces, stats->vertices, stats->passChanges, stats->stateChanges);
	}

	if (name == "RenderStatsResolution")
	{
		return RBX::format("%u x %u", visualEngine->getViewWidth(), visualEngine->getViewHeight());
	}

    if (name == "RenderStatsTimeTotal")
    {
		FrameRateManager* frm = visualEngine->getFrameRateManager();
        double totalWork = frm->GetRenderTimeAverage();

		double prepareAve = frm->GetPrepareTimeAverage();
		double performAve = performAverage.getStats().average;
		double presentAve = presentAverage.getStats().average;

        return RBX::format("%.1f ms (prepare %.1f, perform %.1f, present %.1f, bonus %.1f)",
            totalWork,
            prepareAve,
            performAve,
            presentAve,
            artificialDelay);
    }

	if (name == "RenderStatsDelta")
	{
		FrameRateManager* frm = visualEngine->getFrameRateManager();
		double totalWork = frm->GetRenderTimeAverage();

		double prepareAve = frm->GetPrepareTimeAverage();
		double performAve = performAverage.getStats().average;
		double presentAve = presentAverage.getStats().average;
		double workAve = prepareAve + performAve + presentAve;

		double frameTime = frm->GetFrameTimeAverage();

		return RBX::format("%.1f ms (work %.1f, marshal %.1f, idle %.1f)",
			frameTime,
            workAve,
            workAve > totalWork ? 0.0 : totalWork - workAve,
			totalWork > frameTime ? 0.0 : frameTime - totalWork);
	}
	
	if (name == "RenderStatsGeometryGen")
	{
		RBX::RenderStats* stats = visualEngine->getRenderStats();
		SceneUpdater* su = visualEngine->getSceneUpdater();

		return RBX::format("fast %dc %dp mega %dc queue %dc",
			stats->lastFrameFast.clusters, stats->lastFrameFast.parts,
			stats->lastFrameMegaClusterChunks,
			int(su->getUpdateQueueSize()));
	}
	
	if (name == "RenderStatsClusters")
	{
		RBX::RenderStats* stats = visualEngine->getRenderStats();

		return RBX::format("fw %dc %dp; dyn %dc %dp; hum %dc %dp",
			stats->clusterFastFW.clusters, stats->clusterFastFW.parts,
			stats->clusterFast.clusters, stats->clusterFast.parts,
			stats->clusterFastHumanoid.clusters, stats->clusterFastHumanoid.parts);
	}
	
	if (name == "RenderStatsFRMConfig")
	{
		FrameRateManager* frm = visualEngine->getFrameRateManager();
		
		return RBX::format("level %d (auto %s)",
			frm->GetQualityLevel(), visualEngine->getSettings()->getQualityLevel() == CRenderSettings::QualityAuto ? "on" : "off");
	}

	if (name == "RenderStatsFRMBlocks")
	{
		FrameRateManager* frm = visualEngine->getFrameRateManager();
		
		return RBX::format("%d (target %d)", (int)frm->GetVisibleBlockCounter(), (int)frm->GetVisibleBlockTarget());
	}

	if (name == "RenderStatsFRMDistance")
	{
		FrameRateManager* frm = visualEngine->getFrameRateManager();

		return RBX::format("render %d view %d (...%d)", (int)sqrt(frm->GetRenderCullSqDistance()), (int)sqrt(frm->GetViewCullSqDistance()), frm->GetRecomputeDistanceDelay());
	}

	if (name == "RenderStatsFRMAdjust")
	{
		FrameRateManager* frm = visualEngine->getFrameRateManager();
		
		return RBX::format("up %d down %d backoff %d backoff avg %d", frm->GetQualityDelayUp(), frm->GetQualityDelayDown(), frm->GetBackoffCounter(), (int)frm->GetBackoffAverage());
	}

	if (name == "RenderStatsFRMTargetTime")
	{
		FrameRateManager* frm = visualEngine->getFrameRateManager();

        if (frm->GetQualityLevel() > 0 && frm->GetQualityLevel() < CRenderSettings::QualityLevelMax-1)
            return RBX::format("frame %.1f render %1.f throttle %u", frm->GetTargetFrameTimeForNextLevel(), frm->GetTargetRenderTimeForNextLevel(), frm->getPhysicsThrottling());
        else
            return RBX::format("frame n/a render n/a throttle %u", frm->getPhysicsThrottling());
	}

	if (name == "RenderStatsTC")
	{
		TextureCompositor* tc = visualEngine->getTextureCompositor();
		
		const TextureCompositorConfiguration& config = tc->getConfiguration();
		const TextureCompositorStats& stats = tc->getStatistics();
		
		return RBX::format("%dM hq %d (%dM) lq %d (%dM) cache %d (%dM)",
			config.budget / 1048576,
			stats.liveHQCount, stats.liveHQSize / 1048576,
			stats.liveLQCount, stats.liveLQSize / 1048576,
			stats.orphanedCount, stats.orphanedSize / 1048576);
	}

    if (name == "RenderStatsTM")
	{
		const TextureManagerStats& stats = visualEngine->getTextureManager()->getStatistics();

		return RBX::format("queued %d live %d (%dM) cache %d (%dM)",
			stats.queuedCount,
			stats.liveCount, stats.liveSize / 1048576,
			stats.orphanedCount, stats.orphanedSize / 1048576);
	}

	if (name == "RenderStatsLightGrid")
	{
		SceneUpdater* su = visualEngine->getSceneUpdater();
		const RBX::WindowAverage<double,double>::Stats& stats = su->getLightingTimeStats();
		
		if (su->isLightingActive())
			return RBX::format("%d (occupancy %d, oldest: %d), %.1f ms (std: %.1f)", su->getLastLightingUpdates(), su->getLastOccupancyUpdates(), su->getLightOldestAge(), stats.average, sqrt(stats.variance));
		else
			return "inactive";
	}
	
	if (name == "RenderStatsGPU")
	{
		return RBX::format("%.1f ms (present: %.1f ms) %s", gpuAverage.getStats().average, presentAverage.getStats().average, visualEngine->getDevice()->getAPIName().c_str());
	}
	
	return "";
}

void RenderView::captureMetrics(RBX::RenderMetrics& metrics)
{
	FrameRateManager* frm = visualEngine->getFrameRateManager();

	metrics.presentTime = presentAverage.getStats().average;
	metrics.GPUDelay = gpuAverage.getStats().average;
	metrics.deltaAve = frm->GetFrameTimeAverage();

	WindowAverage<double, double>::Stats renderStats = frm->GetRenderTimeStats().getSanitizedStats();

	metrics.renderAve = renderStats.average;
	GetConfidenceInterval(renderStats.average, renderStats.variance, C90, &metrics.renderConfidenceMin, &metrics.renderConfidenceMax);
	metrics.renderStd = sqrt(renderStats.variance);
}

void RenderView::printScene()
{
}

void RenderView::reloadShaders()
{
	visualEngine->reloadShaders();
}

struct ProxyMetric: IMetric
{
    RenderView* view;
    IMetric* parent;

    ProxyMetric(RenderView* view, IMetric* parent)
        : view(view), parent(parent)
    {
    }

    virtual std::string getMetric(const std::string& metric) const
    {
        if (metric.compare(0, 11, "RenderStats") == 0)
            return view->getRenderStatsMetric(metric);
        else
            return parent->getMetric(metric);
    }

    virtual double getMetricValue(const std::string& metric) const
    {
        if(metric.compare(0, 3, "FRM") == 0)
            return view->getFrameRateManager()->getMetricValue(metric);
        else
            return parent->getMetricValue(metric);
    }
};

void RenderView::renderPrepare(IMetric* metric)
{
	renderPrepareImpl(metric, /* updateViewport= */ true);
}

void RenderView::renderPrepareImpl(IMetric* metric, bool updateViewport)
{
	RBXPROFILER_SCOPE("Render", "Prepare");
    
	Timer<Time::Precise> timer;

	FrameRateManager* frm = visualEngine->getFrameRateManager();

	fogEndTargetFRM = sqrt(frm->GetRenderCullSqDistance()); // filters out a glitch when manually switching FRM level

	frm->SubmitCurrentFrame( deltaMs, totalRenderTime, prepareTime, artificialDelay);

    if (dataModel->getWorkspace())
        dataModel->getWorkspace()->renderingDistance = fogEndTargetFRM;

	if (frameRateManagerStatsItem)
		frameRateManagerStatsItem->updateData(frm);

    visualEngine->reloadQueuedAssets();

	double newtimeMs = Time::now(Time::Precise).timestampSeconds() * 1000;
	if(timestampMs != 0)
	{
		deltaMs = (newtimeMs - timestampMs);
	}
	timestampMs = newtimeMs;

    if (!visualEngine->getDevice()->validate())
        return;

    if (updateViewport)
	{
		Vector2 newViewport = computeCanvasSize(visualEngine->getDevice());

        visualEngine->setViewport(newViewport.x, newViewport.y);

		if (Camera* camera = dataModel->getWorkspace()->getCamera())
		{
			// HACK: Ideally we'd update Camera viewport in renderStep since this way renderStep has data that's one frame behind
			DataModel::scoped_write_transfer request(dataModel.get());

			camera->setViewport(newViewport);
		}
	}

    dataModel->getWorkspace()->getWorld()->setFRMThrottle(frm->getPhysicsThrottling());

    Lighting* lighting = ServiceProvider::find<Lighting>(dataModel.get());
    RBXASSERT(lighting);

    updateFog();
    visualEngine->getSceneManager()->trackLightingTimeOfDay( lighting->getGameTime() );

    Workspace* workspace = dataModel->getWorkspace();
    RBX::Camera* cameraobj = workspace->getCamera();

	Vector3 poi;

	if (cameraobj->getCameraSubject())
	{
		poi = cameraobj->getCameraFocus().translation;
	}
	else
	{
		poi = cameraobj->getCameraCoordinateFrame().translation;
	}

    visualEngine->setCamera(*cameraobj, poi);

    Adorn* adorn = visualEngine->getAdorn();

    adorn->preSubmitPass();

    if (adornsEnabled && !FFlag::DebugAdornsDisabled)
    {
		// HACK: Rendering code should NOT invoke Lua code or change DM in any way. But it does (in ScreenGui & Frame objects inside SurfaceGui).
		DataModel::scoped_write_transfer request(dataModel.get());

        // do 3d adorns first (they will actually be drawn after normal 3D pass)
        dataModel->renderPass3dAdorn(adorn);

        // then do 2d adorns, after 3d adorns (so that UI text shows on top)
        ProxyMetric proxyMetric(this, metric);

		if (FFlag::RenderUIAs3DInVR && adorn->isVR())
		{
			Rect2D viewport = adorn->getViewport();

			CoordinateFrame cframe = cameraobj->getRenderingCoordinateFrame();

			float uiFovTan = tanf(G3D::toRadians(kVRUIFOV / 2));
			float uiDepth = kVRUIDepth;
			float uiSize = uiDepth * uiFovTan * 2;

			cframe.translation += cframe.lookVector() * uiDepth;
			cframe.translation -= cframe.rightVector() * uiSize * 0.5f;
			cframe.translation += cframe.upVector() * uiSize * 0.5f;
			cframe.rotation *= Matrix3::fromDiagonal(Vector3(uiSize / viewport.width(), uiSize / viewport.height(), 1));

			AdornBillboarder adornView(adorn, viewport, cframe, /* alwaysOnTop= */ true);

			dataModel->renderPass2d(&adornView, &proxyMetric);
		}
		else
		{
			dataModel->renderPass2d(adorn, &proxyMetric);
		}
    }

    adorn->postSubmitPass();

    updateLighting(lighting);

    float dt = frm->GetFrameTimeStats().getLatest() / 1000.f;

    visualEngine->getWater()->update(dataModel.get(), dt);

    visualEngine->getSceneUpdater()->updatePrepare(0, *visualEngine->getUpdateFrustum());

    visualEngine->getTextureCompositor()->update(poi);
    
#if defined(RBX_PLATFORM_DURANGO)
    if(RBX::PlatformService* platformService = ServiceProvider::find<PlatformService>(dataModel.get()))
    {
        presetPostProcess(platformService);
    }
#endif
    
    // Pass FRM configuration to shaders
    GlobalShaderData& globalShaderData = visualEngine->getSceneManager()->writeGlobalShaderData();

    float shadingDistance = std::max(frm->getShadingDistance(), 0.1f);
    float fovCoefficient = 	visualEngine->getCamera().getProjectionMatrix()[1][1];
    float outlineScaling =  fovCoefficient * visualEngine->getViewHeight() * (10.0f / FInt::OutlineThickness) * (1/2.0f) / shadingDistance;

    float brightnessMulFactor = outlinesEnabled ? (FInt::OutlineBrightnessMax - FInt::OutlineBrightnessMin) / 255.0f : 0.0f;
    float brightnessAddFactor = outlinesEnabled ? FInt::OutlineBrightnessMin / 255.0f : 1.0f;

    float shadowFade = powf(G3D::clamp(lighting->getSkyParameters().lightDirection.unit().y, 0.f, 1.f), 1.f / 4);
    float shadowIntensity = shadowFade * (FInt::RenderShadowIntensity / 100.f);

    globalShaderData.FadeDistance_GlowFactor = Vector4(shadingDistance, 1.f / shadingDistance, outlineScaling, globalShaderData.FadeDistance_GlowFactor.w);
    globalShaderData.OutlineBrightness_ShadowInfo = Vector4(brightnessMulFactor, brightnessAddFactor, 0, shadowIntensity);
    
    FASTLOG(FLog::ViewRbxBase, "Render prepare end");
    prepareTime = timer.delta().msec();
    prepareAverage.sample(prepareTime);
}

void RenderView::drawRecordingFrame(DeviceContext* context)
{
    RBXASSERT(videoFrameVertexStreamer);

    Framebuffer* fb = visualEngine->getDevice()->getMainFramebuffer();

    static const unsigned lineWidth = 2;
    unsigned w = fb->getWidth();
    unsigned h = fb->getHeight();

    Rect2D screen = Rect2D::xywh(0, 0, w, h);
    Rect2D paddedScreen = Rect2D::xywh(lineWidth, lineWidth, w - lineWidth * 2, h - lineWidth * 2);

    videoFrameVertexStreamer->rectBlt(shared_ptr<Texture>(), Color4(1,0,0), screen.corner(3), paddedScreen.corner(3), screen.corner(0), paddedScreen.corner(0), Vector2::zero(), Vector2::zero(), BatchTextureType_Color, true);
    videoFrameVertexStreamer->rectBlt(shared_ptr<Texture>(), Color4(1,0,0), paddedScreen.corner(2), screen.corner(2), paddedScreen.corner(1), screen.corner(1), Vector2::zero(), Vector2::zero(), BatchTextureType_Color, true);
    videoFrameVertexStreamer->rectBlt(shared_ptr<Texture>(), Color4(1,0,0), paddedScreen.corner(0), paddedScreen.corner(1), screen.corner(0), screen.corner(1), Vector2::zero(), Vector2::zero(), BatchTextureType_Color, true);
    videoFrameVertexStreamer->rectBlt(shared_ptr<Texture>(), Color4(1,0,0), screen.corner(3), screen.corner(2), paddedScreen.corner(3), paddedScreen.corner(2), Vector2::zero(), Vector2::zero(), BatchTextureType_Color, true);

    videoFrameVertexStreamer->renderPrepare();
    videoFrameVertexStreamer->render2D(context, w, h, visualEngine->getRenderStats()->passUI);
    videoFrameVertexStreamer->renderFinish();
}

void RenderView::drawVRWindow(DeviceContext* context)
{
	RBXPROFILER_SCOPE("GPU", "Window");

	DeviceVR* vr = visualEngine->getDevice()->getVR();
    Framebuffer* mainFramebuffer = visualEngine->getDevice()->getMainFramebuffer();
	Framebuffer* eyeFramebuffer = vr->getEyeFramebuffer(0);

	if (!vrVertexStreamer)
		vrVertexStreamer.reset(new VertexStreamer(visualEngine.get()));

	if (!vrDebugTexture || vrDebugTexture->getWidth() != eyeFramebuffer->getWidth() || vrDebugTexture->getHeight() != eyeFramebuffer->getHeight())
		vrDebugTexture = visualEngine->getDevice()->createTexture(Texture::Type_2D, Texture::Format_RGBA8, eyeFramebuffer->getWidth(), eyeFramebuffer->getHeight(), 1, 1, Texture::Usage_Renderbuffer);

	int w = mainFramebuffer->getWidth();
    int h = mainFramebuffer->getHeight();

	int x = (w - h) / 2;

	const float clearColor[4] = {};

	context->copyFramebuffer(eyeFramebuffer, vrDebugTexture.get());

	context->bindFramebuffer(mainFramebuffer);
	context->clearFramebuffer(DeviceContext::Buffer_Color, clearColor, 1.f, 0);

	vrVertexStreamer->rectBlt(vrDebugTexture, Color4(1,1,1),
		Vector2(x,h), Vector2(w - x,h),
		Vector2(x,0), Vector2(w - x,0),
		Vector2(0,0), Vector2(1,1), BatchTextureType_Color, false);

	vrVertexStreamer->renderPrepare();
    vrVertexStreamer->render2D(context, w, h, visualEngine->getRenderStats()->passUI);
    vrVertexStreamer->renderFinish();
}

namespace
{
	const int g_MicroProfileFontTextureX = 1024;
	const int g_MicroProfileFontTextureY = 9;

	const uint16_t g_MicroProfileFontDescription[] =
	{
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x201,0x209,0x211,0x219,0x221,0x229,0x231,0x239,0x241,0x249,0x251,0x259,0x261,0x269,0x271,
		0x1b1,0x1b9,0x1c1,0x1c9,0x1d1,0x1d9,0x1e1,0x1e9,0x1f1,0x1f9,0x279,0x281,0x289,0x291,0x299,0x2a1,
		0x2a9,0x001,0x009,0x011,0x019,0x021,0x029,0x031,0x039,0x041,0x049,0x051,0x059,0x061,0x069,0x071,
		0x079,0x081,0x089,0x091,0x099,0x0a1,0x0a9,0x0b1,0x0b9,0x0c1,0x0c9,0x2b1,0x2b9,0x2c1,0x2c9,0x2d1,
		0x0ce,0x0d9,0x0e1,0x0e9,0x0f1,0x0f9,0x101,0x109,0x111,0x119,0x121,0x129,0x131,0x139,0x141,0x149,
		0x151,0x159,0x161,0x169,0x171,0x179,0x181,0x189,0x191,0x199,0x1a1,0x2d9,0x2e1,0x2e9,0x2f1,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
	};

	const uint8_t g_MicroProfileFont[] = 
	{
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x10,0x78,0x38,0x78,0x7c,0x7c,0x3c,0x44,0x38,0x04,0x44,0x40,0x44,0x44,0x38,0x78,
		0x38,0x78,0x38,0x7c,0x44,0x44,0x44,0x44,0x44,0x7c,0x00,0x00,0x40,0x00,0x04,0x00,
		0x18,0x00,0x40,0x10,0x08,0x40,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x10,0x38,0x7c,0x08,0x7c,0x1c,0x7c,0x38,0x38,
		0x10,0x28,0x28,0x10,0x00,0x20,0x10,0x08,0x10,0x10,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x04,0x00,0x20,0x38,0x38,0x70,0x00,0x1c,0x10,0x00,0x1c,0x10,0x70,0x30,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x28,0x44,0x44,0x44,0x40,0x40,0x40,0x44,0x10,0x04,0x48,0x40,0x6c,0x44,0x44,0x44,
		0x44,0x44,0x44,0x10,0x44,0x44,0x44,0x44,0x44,0x04,0x00,0x00,0x40,0x00,0x04,0x00,
		0x24,0x00,0x40,0x00,0x00,0x40,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x44,0x30,0x44,0x04,0x18,0x40,0x20,0x04,0x44,0x44,
		0x10,0x28,0x28,0x3c,0x44,0x50,0x10,0x10,0x08,0x54,0x10,0x00,0x00,0x00,0x04,0x00,
		0x00,0x08,0x00,0x10,0x44,0x44,0x40,0x40,0x04,0x28,0x00,0x30,0x10,0x18,0x58,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x44,0x44,0x40,0x44,0x40,0x40,0x40,0x44,0x10,0x04,0x50,0x40,0x54,0x64,0x44,0x44,
		0x44,0x44,0x40,0x10,0x44,0x44,0x44,0x28,0x28,0x08,0x00,0x38,0x78,0x3c,0x3c,0x38,
		0x20,0x38,0x78,0x30,0x18,0x44,0x10,0x6c,0x78,0x38,0x78,0x3c,0x5c,0x3c,0x3c,0x44,
		0x44,0x44,0x44,0x44,0x7c,0x00,0x4c,0x10,0x04,0x08,0x28,0x78,0x40,0x08,0x44,0x44,
		0x10,0x00,0x7c,0x50,0x08,0x50,0x00,0x20,0x04,0x38,0x10,0x00,0x00,0x00,0x08,0x10,
		0x10,0x10,0x7c,0x08,0x08,0x54,0x40,0x20,0x04,0x44,0x00,0x30,0x10,0x18,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x44,0x78,0x40,0x44,0x78,0x78,0x40,0x7c,0x10,0x04,0x60,0x40,0x54,0x54,0x44,0x78,
		0x44,0x78,0x38,0x10,0x44,0x44,0x54,0x10,0x10,0x10,0x00,0x04,0x44,0x40,0x44,0x44,
		0x78,0x44,0x44,0x10,0x08,0x48,0x10,0x54,0x44,0x44,0x44,0x44,0x60,0x40,0x10,0x44,
		0x44,0x44,0x28,0x44,0x08,0x00,0x54,0x10,0x18,0x18,0x48,0x04,0x78,0x10,0x38,0x3c,
		0x10,0x00,0x28,0x38,0x10,0x20,0x00,0x20,0x04,0x10,0x7c,0x00,0x7c,0x00,0x10,0x00,
		0x00,0x20,0x00,0x04,0x10,0x5c,0x40,0x10,0x04,0x00,0x00,0x60,0x10,0x0c,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x7c,0x44,0x40,0x44,0x40,0x40,0x4c,0x44,0x10,0x04,0x50,0x40,0x44,0x4c,0x44,0x40,
		0x54,0x50,0x04,0x10,0x44,0x44,0x54,0x28,0x10,0x20,0x00,0x3c,0x44,0x40,0x44,0x7c,
		0x20,0x44,0x44,0x10,0x08,0x70,0x10,0x54,0x44,0x44,0x44,0x44,0x40,0x38,0x10,0x44,
		0x44,0x54,0x10,0x44,0x10,0x00,0x64,0x10,0x20,0x04,0x7c,0x04,0x44,0x20,0x44,0x04,
		0x10,0x00,0x7c,0x14,0x20,0x54,0x00,0x20,0x04,0x38,0x10,0x10,0x00,0x00,0x20,0x10,
		0x10,0x10,0x7c,0x08,0x10,0x58,0x40,0x08,0x04,0x00,0x00,0x30,0x10,0x18,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x44,0x44,0x44,0x44,0x40,0x40,0x44,0x44,0x10,0x44,0x48,0x40,0x44,0x44,0x44,0x40,
		0x48,0x48,0x44,0x10,0x44,0x28,0x6c,0x44,0x10,0x40,0x00,0x44,0x44,0x40,0x44,0x40,
		0x20,0x3c,0x44,0x10,0x08,0x48,0x10,0x54,0x44,0x44,0x44,0x44,0x40,0x04,0x12,0x4c,
		0x28,0x54,0x28,0x3c,0x20,0x00,0x44,0x10,0x40,0x44,0x08,0x44,0x44,0x20,0x44,0x08,
		0x00,0x00,0x28,0x78,0x44,0x48,0x00,0x10,0x08,0x54,0x10,0x10,0x00,0x00,0x40,0x00,
		0x10,0x08,0x00,0x10,0x00,0x40,0x40,0x04,0x04,0x00,0x00,0x30,0x10,0x18,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x44,0x78,0x38,0x78,0x7c,0x40,0x3c,0x44,0x38,0x38,0x44,0x7c,0x44,0x44,0x38,0x40,
		0x34,0x44,0x38,0x10,0x38,0x10,0x44,0x44,0x10,0x7c,0x00,0x3c,0x78,0x3c,0x3c,0x3c,
		0x20,0x04,0x44,0x38,0x48,0x44,0x38,0x44,0x44,0x38,0x78,0x3c,0x40,0x78,0x0c,0x34,
		0x10,0x6c,0x44,0x04,0x7c,0x00,0x38,0x38,0x7c,0x38,0x08,0x38,0x38,0x20,0x38,0x70,
		0x10,0x00,0x28,0x10,0x00,0x34,0x00,0x08,0x10,0x10,0x00,0x20,0x00,0x10,0x00,0x00,
		0x20,0x04,0x00,0x20,0x10,0x3c,0x70,0x00,0x1c,0x00,0x7c,0x1c,0x10,0x70,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x38,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x40,0x04,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	};
}

class ProfilerRenderer: public Profiler::Renderer
{
public:
	ProfilerRenderer(VisualEngine* visualEngine)
		: visualEngine(visualEngine)
		, colorOrderBGR(false)
	{
		Device* device = visualEngine->getDevice();

		colorOrderBGR = device->getCaps().colorOrderBGR;

		std::vector<VertexLayout::Element> elements;
		elements.push_back(VertexLayout::Element(0, offsetof(Vertex, x), VertexLayout::Format_Float2, VertexLayout::Semantic_Position));
		elements.push_back(VertexLayout::Element(0, offsetof(Vertex, u), VertexLayout::Format_Float2, VertexLayout::Semantic_Texture));
		elements.push_back(VertexLayout::Element(0, offsetof(Vertex, color), VertexLayout::Format_Color, VertexLayout::Semantic_Color));

		layout = device->createVertexLayout(elements);

		const size_t fontSize = g_MicroProfileFontTextureX * g_MicroProfileFontTextureY * 4;

		uint32_t* pUnpacked = (uint32_t*)alloca(fontSize);

		int idx = 0;
		int end = g_MicroProfileFontTextureX * g_MicroProfileFontTextureY / 8;
		for (int i = 0; i < end; i++)
		{
			unsigned char pValue = g_MicroProfileFont[i];
			for (int j = 0; j < 8; ++j)
			{
				pUnpacked[idx++] = (pValue & 0x80) ? 0xffffffff : 0;
				pValue <<= 1;
			}
		}

		pUnpacked[idx-1] = 0xffffffff;

		fontTexture = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, g_MicroProfileFontTextureX, g_MicroProfileFontTextureY, 1, 1, Texture::Usage_Static);

		fontTexture->upload(0, 0, TextureRegion(0, 0, g_MicroProfileFontTextureX, g_MicroProfileFontTextureY), pUnpacked, fontSize);
	}

	void drawText(int x, int y, unsigned int color, const char* text, unsigned int length, unsigned int textWidth, unsigned int textHeight) override
	{
		const float fOffsetU = float(textWidth) / float(g_MicroProfileFontTextureX);

		float fX = (float)x;
		float fY = (float)y;
		float fY2 = fY + (textHeight+1);

		const char* pStr = text;

		if (!colorOrderBGR)
			color = 0xff000000|((color&0xff)<<16)|(color&0xff00)|((color>>16)&0xff);

		for (uint32_t j = 0; j < length; ++j)
		{
			int16_t nOffset = g_MicroProfileFontDescription[uint8_t(*pStr++)];
			float fOffset = float(nOffset) / float(g_MicroProfileFontTextureX);

			Vertex v0 = { fX, fY, fOffset, 0.f, color };
			Vertex v1 = { fX + textWidth, fY, fOffset + fOffsetU, 0.f, color };
			Vertex v2 = { fX + textWidth, fY2, fOffset + fOffsetU, 1.f, color };
			Vertex v3 = { fX, fY2, fOffset, 1.f, color };

			quads.push_back(v0);
			quads.push_back(v1);
			quads.push_back(v3);
			quads.push_back(v1);
			quads.push_back(v2);
			quads.push_back(v3);

			fX += textWidth+1;
		}
	}

	void drawBox(int x0, int y0, int x1, int y1, unsigned int color0, unsigned int color1) override
	{
		if (!colorOrderBGR)
		{
			color0 = ((color0&0xff)<<16)|((color0>>16)&0xff)|(0xff00ff00&color0);
			color1 = ((color1&0xff)<<16)|((color1>>16)&0xff)|(0xff00ff00&color1);
		}

		Vertex v0 = { (float)x0, (float)y0, 2.f, 2.f, color0 };
		Vertex v1 = { (float)x1, (float)y0, 2.f, 2.f, color0 };
		Vertex v2 = { (float)x1, (float)y1, 2.f, 2.f, color1 };
		Vertex v3 = { (float)x0, (float)y1, 2.f, 2.f, color1 };

		quads.push_back(v0);
		quads.push_back(v1);
		quads.push_back(v3);
		quads.push_back(v1);
		quads.push_back(v2);
		quads.push_back(v3);
	}

	void drawLine(unsigned int vertexCount, const float* vertexData, unsigned int color) override
	{
		if (!colorOrderBGR)
			color = ((color&0xff)<<16)|((color>>16)&0xff)|(0xff00ff00&color);

		for (unsigned int i = 0; i + 1 < vertexCount; ++i)
		{
			Vertex v0 = { vertexData[2 * i + 0], vertexData[2 * i + 1], 2.f, 2.f, color };
			Vertex v1 = { vertexData[2 * i + 2], vertexData[2 * i + 3], 2.f, 2.f, color };

			lines.push_back(v0);
			lines.push_back(v1);
		}
	}

	void flush(DeviceContext* context, const RenderCamera& camera)
	{
		if (quads.empty() && lines.empty())
			return;

		updateBuffer(quadsVB, quadsGeometry, quads);
		updateBuffer(linesVB, linesGeometry, lines);

		if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("ProfilerVS", "ProfilerFS"))
		{
			GlobalShaderData shaderData = visualEngine->getSceneManager()->readGlobalShaderData();

			shaderData.setCamera(camera);

			context->updateGlobalConstants(&shaderData, sizeof(GlobalShaderData));

			context->bindProgram(program.get());

			context->setRasterizerState(RasterizerState::Cull_None);
			context->setBlendState(BlendState::Mode_AlphaBlend);
			context->setDepthState(DepthState(DepthState::Function_Always, false));

			context->bindTexture(0, fontTexture.get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

			if (!quads.empty())
				context->draw(quadsGeometry.get(), Geometry::Primitive_Triangles, 0, quads.size(), 0, quads.size());

			if (!lines.empty())
				context->draw(linesGeometry.get(), Geometry::Primitive_Lines, 0, lines.size(), 0, lines.size());
		}

		quads.clear();
		lines.clear();
	}

private:
	struct Vertex
	{
		float x, y;
		float u, v;
		unsigned int color;
	};

	VisualEngine* visualEngine;

	bool colorOrderBGR;

	std::vector<Vertex> quads;
	std::vector<Vertex> lines;

	shared_ptr<Texture> fontTexture;

	shared_ptr<VertexLayout> layout;

	shared_ptr<VertexBuffer> quadsVB;
	shared_ptr<Geometry> quadsGeometry;

	shared_ptr<VertexBuffer> linesVB;
	shared_ptr<Geometry> linesGeometry;

	void updateBuffer(shared_ptr<VertexBuffer>& vb, shared_ptr<Geometry>& geometry, const std::vector<Vertex>& vertices)
	{
		if (vertices.empty()) return;

		if (!vb || vb->getElementCount() < vertices.size())
		{
			size_t count = 1;
			while (count < vertices.size())
				count *= 2;

			vb = visualEngine->getDevice()->createVertexBuffer(sizeof(Vertex), count, GeometryBuffer::Usage_Dynamic);
			geometry = visualEngine->getDevice()->createGeometry(layout, vb, shared_ptr<IndexBuffer>());
		}

		void* locked = vb->lock(VertexBuffer::Lock_Discard);
		memcpy(locked, vertices.data(), vertices.size() * sizeof(Vertex));
		vb->unlock();
	}
};

void RenderView::drawProfiler(DeviceContext* context)
{
	if (!Profiler::isVisible())
		return;

	if (!profilerRenderer)
		profilerRenderer.reset(new ProfilerRenderer(visualEngine.get()));

    RBXPROFILER_SCOPE("GPU", "Profiler");

    Framebuffer* fb = visualEngine->getDevice()->getMainFramebuffer();

	Profiler::render(profilerRenderer.get(), fb->getWidth(), fb->getHeight());

	RenderCamera camera;
	camera.setViewMatrix(Matrix4::identity());
	camera.setProjectionOrtho(fb->getWidth(), fb->getHeight(), -1, 1, visualEngine->getDevice()->getCaps().needsHalfPixelOffset);

	profilerRenderer->flush(context, camera);
}

void RenderView::renderPerform(double timeJobStart)
{
	renderPerformImpl(timeJobStart, visualEngine->getDevice()->getMainFramebuffer());
}

void RenderView::renderPerformImpl(double timeJobStart, Framebuffer* mainFramebuffer)
{
	RBXPROFILER_SCOPE("Render", "Perform");

    if( !this->dataModel )
        return;

	Timer<Time::Precise> timer;

	FASTLOG(FLog::ViewRbxBase, "Render perform start");

    double performTime = 0;
    double presentTime = 0;

    Adorn* adorn = visualEngine->getAdorn();

    // update glyph atlas
    if (FFlag::UseDynamicTypesetterUTF8)
        visualEngine->getGlyphAtlas()->upload();

    adorn->prepareRenderPass();

    if (DeviceContext* context = visualEngine->getDevice()->beginFrame())
    {
        visualEngine->getSceneUpdater()->updatePerform();

        visualEngine->getMaterialGenerator()->garbageCollectIncremental();
	    visualEngine->getTextureManager()->garbageCollectIncremental();

	    visualEngine->getTextureManager()->processPendingRequests();

		context->setDefaultAnisotropy(std::max(1, visualEngine->getFrameRateManager()->getTextureAnisotropy()));

        visualEngine->getTextureCompositor()->render(context);

		if (DeviceVR* vr = visualEngine->getDevice()->getVR())
		{
			RBXPROFILER_SCOPE("GPU", "Scene");

			DeviceVR::State vrState = vr->getState();

            const RenderCamera& cullCamera = visualEngine->getCameraCull();

            float zNear = 0.5f;
            float zFar = 5000.f;

			visualEngine->getSceneManager()->renderBegin(context, cullCamera, vr->getEyeFramebuffer(0)->getWidth(), vr->getEyeFramebuffer(0)->getHeight());

			for (int eye = 0; eye < 2; ++eye)
			{
				RBXPROFILER_SCOPE("GPU", "Eye");

				Framebuffer* eyeFB = vr->getEyeFramebuffer(eye);

				Vector3 eyeOffset = Vector3(vrState.eyeOffset[eye]) * kVRScale;

				RenderCamera eyeCamera = visualEngine->getCamera();

				eyeCamera.setViewMatrix(Matrix4::translation(-eyeOffset) * eyeCamera.getViewMatrix());
				eyeCamera.setProjectionPerspective(vrState.eyeFov[eye][0], vrState.eyeFov[eye][1], vrState.eyeFov[eye][2], vrState.eyeFov[eye][3], zNear, zFar);

				visualEngine->getSceneManager()->renderView(context, eyeFB, eyeCamera, eyeFB->getWidth(), eyeFB->getHeight());
			}

			visualEngine->getSceneManager()->renderEnd(context);

            if (vrState.needsMirror)
                drawVRWindow(context);
		}
		else
		{
            visualEngine->getSceneManager()->renderScene(context, mainFramebuffer, visualEngine->getCamera(), visualEngine->getViewWidth(), visualEngine->getViewHeight());
		}

        if ((!DFFlag::ScreenShotDuplicationFix && doScreenshot) || (DFFlag::ScreenShotDuplicationFix && getAndClearDoScreenshot()))
        {
            std::string screenshotFilename;
            if (saveScreenshotToFile(screenshotFilename))
            {
                if (dataModel)
                {
                    dataModel->submitTask(boost::bind(&RBX::DataModel::ScreenshotReadyTask, weak_ptr<RBX::DataModel>(dataModel), screenshotFilename), RBX::DataModelJob::Write);
                }
            }
        }

        performTime = timer.reset().msec();
		performAverage.sample(performTime);

        if (videoFrameVertexStreamer && frameDataCallback)
        {
            frameDataCallback(visualEngine->getDevice());
            // this has to be checked again, as frameDataCallback can stop video recording and NULL it
            if (videoFrameVertexStreamer)
                drawRecordingFrame(context);
        }

		drawProfiler(context);

		RBXPROFILER_SCOPE("Render", "Present");

		visualEngine->getDevice()->endFrame();

        presentTime = timer.delta().msec();
		presentAverage.sample(presentTime);

		gpuAverage.sample(visualEngine->getDevice()->getStatistics().gpuFrameTime);
    }

    adorn->finishRenderPass();

	totalRenderTime = (Time::nowFastSec() - timeJobStart)*1000.0;

	FASTLOG4F(FLog::ViewRbxBase, "Render perform end. Delta: %f, Total %f, Present: %f, Prepare: %f", deltaMs, totalRenderTime, presentTime, prepareTime);
}

void RenderView::buildGui(bool buildInGameGui)
{
	dataModel->startCoreScripts(buildInGameGui);
}

RenderStats & RenderView::getRenderStats() { return *visualEngine->getRenderStats(); }

void RenderView::presetLighting(RBX::Lighting* l, const RBX::Color3& extraAmbient, float skylightFactor)
{
	outlinesEnabled = l->getOutlines();

	const G3D::LightingParameters& skyParameters = l->getSkyParameters();
	SceneManager *smgr = visualEngine->getSceneManager();
	
	Color3 bottomColorShift = l->getBottomColorShift();
	Color3 topColorShift = l->getTopColorShift();

	float bottomShiftBase = (bottomColorShift.r+bottomColorShift.g+bottomColorShift.b)*0.3333f;
	Color3 bottomAdjustment = bottomColorShift - Color3(bottomShiftBase,bottomShiftBase, bottomShiftBase);
	
	float topShiftBase = (topColorShift.r+topColorShift.g+topColorShift.b)*0.3333f;
	Color3 topAdjustment = topColorShift - Color3(topShiftBase,topShiftBase, topShiftBase);

	float globalBrightness = G3D::clamp(l->getGlobalBrightness(), 0.05f, 5.0f);

	Vector3 sunDirection = -skyParameters.lightDirection.unit();
	Color3 sunColor = skyParameters.lightColor * 0.9f;

	Color3 ambientColor = l->getGlobalAmbient() + extraAmbient;
	
	Color3 keyLightColor = sunColor * globalBrightness;
	keyLightColor += topAdjustment * keyLightColor.length();
	keyLightColor *= skylightFactor;

	Color3 fillLightColor = sunColor * globalBrightness * 0.4f;
	fillLightColor += bottomAdjustment * fillLightColor.length();
	fillLightColor *= skylightFactor;

	smgr->setLighting(ambientColor, sunDirection, keyLightColor.min(Color3::white()), fillLightColor.min(Color3::white()));
}

void RenderView::presetPostProcess(RBX::PlatformService* platformService)
{
#if defined(RBX_PLATFORM_DURANGO)
    SceneManager *smgr = visualEngine->getSceneManager();

    smgr->setPostProcess(platformService->brightness, platformService->contrast, platformService->grayscaleLevel, platformService->blurIntensity, platformService->tintColor);
#endif
}

static void waitForContent(RBX::ContentProvider* contentProvider)
{
	boost::xtime expirationTime;
	boost::xtime_get(&expirationTime, boost::TIME_UTC_);
	expirationTime.sec += static_cast<boost::xtime::xtime_sec_t>(120);

	// TODO: Bail out after a certain number of seconds...
	while (!contentProvider->isRequestQueueEmpty())
	{
		boost::xtime xt;
		boost::xtime_get(&xt, boost::TIME_UTC_);

		if(xtime_cmp(xt, expirationTime) == 1)
			throw RBX::runtime_error("Timeout while waiting for content - 120 seconds");

        boost::this_thread::sleep(boost::posix_time::milliseconds(20));
	}
}

static void modifyThumbnailCamera(VisualEngine* visualEngine, bool allowDolly, int recdepth = 1)
{
	if (recdepth > 10)
		return;

	std::vector<CullableSceneNode*> nodes = visualEngine->getSceneManager()->getSpatialHashedScene()->getAllNodes();

    Extents extents;

    for (size_t i = 0; i < nodes.size(); ++i)
		if (!dynamic_cast<LightObject*>(nodes[i]))
			extents.expandToContain(nodes[i]->getWorldBounds());

	Matrix4 viewProj = visualEngine->getCamera().getViewProjectionMatrix();

    Extents screen;

    for (unsigned i = 0; i < 8; ++i)
	{
		Vector4 p = viewProj * Vector4(extents.getCorner(i), 1);
		Vector3 q = p.xyz() / p.w;

		if (p.w <= 0.001f || fabsf(q.x) > 1 || fabsf(q.y) > 1 || fabsf(q.z) > 1 )
		{
			if( !allowDolly )
				return;

			Matrix4 view = visualEngine->getCameraMutable().getViewMatrix();
			Matrix4 dolly = Matrix4::translation(0,0,-1-recdepth*recdepth);
			visualEngine->getCameraMutable().setViewMatrix( dolly * view );
			return modifyThumbnailCamera(visualEngine,true,recdepth+1); // try once again
		}

		screen.expandToContain(q);
	}
	
	float zoomH = 2 / (screen.max().x - screen.min().x);
	float zoomV = 2 / (screen.max().y - screen.min().y);
	float zoom = std::min(zoomV, zoomH);

	float offH  = screen.center().x;
	float offV  = screen.center().y;

	Matrix4 crop = Matrix4::identity();

	crop[0][0] = zoom;
	crop[1][1] = zoom;
	crop[0][3] = -zoom * offH;
	crop[1][3] = -zoom * offV;

	visualEngine->getCameraMutable().setProjectionMatrix( crop * visualEngine->getCamera().getProjectionMatrix() );
}

void RenderView::prepareSceneGraph()
{
    // will populate scenegraph, and trigger resource loads.
    visualEngine->getSceneUpdater()->setComputeLightingEnabled(false);

    // wait for all networked resources to load.
    // do this outside scoped lock because this is the IO/network bound task.

    RBX::ContentProvider* contentProvider = visualEngine->getContentProvider();
    RBX::MeshContentProvider* meshContentProvider = visualEngine->getMeshContentProvider();
    bool allContentLoaded = false;

    do
    {
        FASTLOG(FLog::ThumbnailRender, "Waiting for content to load");

        waitForContent(contentProvider);

        RBX::ServiceProvider::find<RBX::Workspace>(dataModel.get())->assemble();

		renderPrepareImpl(NULL, /* updateViewport= */ false);

        // Clear adorn rendering queue
        visualEngine->getAdorn()->finishRenderPass();

        // Load textures
        while (!visualEngine->getTextureManager()->isQueueEmpty())
        {
            visualEngine->getTextureManager()->processPendingRequests();

            boost::this_thread::sleep(boost::posix_time::milliseconds(20));
        }

        // Run texture compositor
        if (!visualEngine->getTextureCompositor()->isQueueEmpty())
        {
            if (DeviceContext* context = visualEngine->getDevice()->beginFrame())
            {
                visualEngine->getTextureCompositor()->render(context);
                visualEngine->getDevice()->endFrame();
            }
        }

        allContentLoaded =
            contentProvider->isRequestQueueEmpty() &&
            meshContentProvider->isRequestQueueEmpty() &&
            !visualEngine->getSceneUpdater()->arePartsWaitingForAssets() &&
            visualEngine->getTextureCompositor()->isQueueEmpty() &&
            visualEngine->getTextureManager()->isQueueEmpty();

        FASTLOG2(FLog::ThumbnailRender, "After render: Providers requests empty. Content: %u, Mesh: %u", 
            contentProvider->isRequestQueueEmpty(), meshContentProvider->isRequestQueueEmpty());

        FASTLOG3(FLog::ThumbnailRender, "Parts not waiting for assets: %u, Texture Compositor Queue Empty: %u, Texture Manager Queue Empty: %u", 
            !visualEngine->getSceneUpdater()->arePartsWaitingForAssets(), visualEngine->getTextureCompositor()->isQueueEmpty(), visualEngine->getTextureManager()->isQueueEmpty());
    } while(!allContentLoaded);

    visualEngine->getSceneUpdater()->setComputeLightingEnabled(true);
}

void RenderView::renderThumb(unsigned char* data, int width, int height, bool crop, bool allowDolly)
{
	// will populate scenegraph, and trigger resource loads.
	FASTLOG(FLog::ThumbnailRender, "Rendering thumbnail: populating scene graph, trigger resource load");
    prepareSceneGraph();

	visualEngine->getFrameRateManager()->SubmitCurrentFrame( 1000, 1000, 1000, 0 );

    // Run find visible objects once to fill in distance for LightGrid
	visualEngine->getSceneManager()->computeMinimumSqDistance(visualEngine->getCamera());

	visualEngine->setViewport(width, height);

	if (Camera* camera = dataModel->getWorkspace()->getCamera())
		camera->setViewport(Vector2int16(width, height));

	renderPrepareImpl(NULL, /* updateViewport= */ false);

	if (crop)
	{
		modifyThumbnailCamera(visualEngine.get(),allowDolly);
	}

	shared_ptr<Renderbuffer> color = visualEngine->getDevice()->createRenderbuffer(Texture::Format_RGBA8, width, height, 1);
	shared_ptr<Renderbuffer> depth = visualEngine->getDevice()->createRenderbuffer(Texture::Format_D24S8, width, height, 1);
	shared_ptr<Framebuffer> framebuffer = visualEngine->getDevice()->createFramebuffer(color, depth);

	renderPerformImpl(0.f, framebuffer.get());

    framebuffer->download(data, width * height * 4);
}

//
//	Write render target to file, using user picture folder\Roblox 
//
bool RenderView::saveScreenshotToFile(std::string& /*output*/ filename)
{
#if defined(RBX_PLATFORM_DURANGO)
    // TODO: implement screenshot for durango
#else
	Framebuffer* framebuffer = visualEngine->getDevice()->getMainFramebuffer();

	if (framebuffer)
	{
        time_t ctTime;
        time(&ctTime);

        struct tm* pTime = localtime(&ctTime);

        std::ostringstream name;
        name << "RobloxScreenShot";
        name << std::setw(2) << std::setfill('0') << (pTime->tm_mon + 1)
            << std::setw(2) << std::setfill('0') << pTime->tm_mday
            << std::setw(2) << std::setfill('0') << (pTime->tm_year + 1900)
            << "_" << std::setw(2) << std::setfill('0') << pTime->tm_hour
            << std::setw(2) << std::setfill('0') << pTime->tm_min
            << std::setw(2) << std::setfill('0') << pTime->tm_sec
            << std::setw(3) << std::setfill('0') << ((clock() * 1000 / CLOCKS_PER_SEC) % 1000);
        name << ".png";

        boost::filesystem::path path = FileSystem::getUserDirectory(true, RBX::DirPicture) / name.str();

        G3D::GImage image(framebuffer->getWidth(), framebuffer->getHeight(), 4);
		framebuffer->download(image.byte(), image.width() * image.height() * 4);

		image.convertToRGB();

        G3D::BinaryOutput binaryOutput;
		image.encode(G3D::GImage::PNG, binaryOutput);

        std::ofstream out(path.native().c_str(), std::ios::out | std::ios::binary);
        out.write(reinterpret_cast<const char*>(binaryOutput.getCArray()), binaryOutput.length());

        // Not Unicode-safe! :(
        filename = path.string();

        return true;
	}
#endif
    return false;
}

void RenderView::queueAssetReload(const std::string& filePath)
{
    visualEngine->queueAssetReload(filePath);
}

void RenderView::immediateAssetReload(const std::string& filePath)
{
	visualEngine->immediateAssetReload(filePath);
}

bool RenderView::exportSceneThumbJSON(ExporterSaveType saveType, ExporterFormat format, bool encodeBase64, std::string& strOut)
{
#if !defined(RBX_PLATFORM_DURANGO)
    FASTLOG(FLog::ThumbnailRender, "Rendering thumbnail: populating scene graph, trigger resource load");
    prepareSceneGraph();

    return ObjectExporter::exportToJSON(saveType, format, dataModel.get(), visualEngine.get(), encodeBase64, &strOut);
#else
    return false;
#endif
}

bool RenderView::exportScene(const std::string& filePath, ExporterSaveType saveType, ExporterFormat format)
{
#if !defined(RBX_PLATFORM_DURANGO)
    return ObjectExporter::exportToFile(filePath, saveType, format, dataModel.get(), visualEngine.get());
#else
    return false;
#endif
}


void RenderView::suspendView()
{
	device->suspend();
}

void RenderView::resumeView()
{
	device->resume();
}

void RenderView::garbageCollect()
{
	visualEngine->getMaterialGenerator()->garbageCollectFull();
	visualEngine->getTextureCompositor()->garbageCollectFull();
    visualEngine->getTextureManager()->garbageCollectFull();

	if (RBX::ContentProvider* contentProvider = visualEngine->getContentProvider())
    {
        contentProvider->clearContent();
    }
    
	if (RBX::MeshContentProvider* meshContentProvider = visualEngine->getMeshContentProvider())
    {
        meshContentProvider->clearContent();
    }
}

std::pair<unsigned, unsigned> RenderView::setFrameDataCallback(const boost::function<void(void*)>& callback)
{
    Framebuffer* framebuffer = visualEngine->getDevice()->getMainFramebuffer();
    if (callback && framebuffer)
    {
        videoFrameVertexStreamer.reset(new VertexStreamer(visualEngine.get()));
        frameDataCallback = callback;
        return std::make_pair(framebuffer->getWidth(), framebuffer->getHeight());
    }
    else
    {
        videoFrameVertexStreamer.reset();
        frameDataCallback = FrameDataCallback();
        return std::make_pair(0, 0);
    }
}

double busyWaitLoop(double ms)
{
    double start = Time::now(Time::Precise).timestampSeconds() * 1000;
    for (;;)
    {
        double cur = Time::now(Time::Precise).timestampSeconds() * 1000;
        if (cur-start > ms)
            return cur-start;
    }
}

}
}
