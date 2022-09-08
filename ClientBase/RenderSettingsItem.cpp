/**
 * RenderSettingsItem.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RenderSettingsItem.h"

// Roblox Headers
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/GameBasicSettings.h"
#include "V8DataModel/PartInstance.h"
#include "util/standardout.h"
#include "rbx/SystemUtil.h"

using namespace RBX;

RBX_REGISTER_CLASS(CRenderSettingsItem);

RBX_REGISTER_ENUM(CRenderSettings::AASamples);
RBX_REGISTER_ENUM(CRenderSettings::GraphicsMode);
RBX_REGISTER_ENUM(CRenderSettings::FrameRateManagerMode);
RBX_REGISTER_ENUM(CRenderSettings::AntialiasingMode);
RBX_REGISTER_ENUM(CRenderSettings::QualityLevel);
RBX_REGISTER_ENUM(CRenderSettings::ResolutionPreset);

namespace RBX
{
	namespace Reflection
	{
		template<> Reflection::EnumDesc<CRenderSettings::AASamples>::EnumDesc()
			:RBX::Reflection::EnumDescriptor("AASamples")
		{
			addPair(CRenderSettings::NONE, "None");
			addPair(CRenderSettings::AA4, "4");
			addPair(CRenderSettings::AA8, "8");
		}

		template<> Reflection::EnumDesc<CRenderSettings::GraphicsMode>::EnumDesc()
			:Reflection::EnumDescriptor("GraphicsMode")
		{
			addPair(CRenderSettings::AutoGraphicsMode,"Automatic");
			addPair(CRenderSettings::Direct3D9,"Direct3D9");
            addPair(CRenderSettings::Direct3D11,"Direct3D11");
			addPair(CRenderSettings::OpenGL,"OpenGL");
			addPair(CRenderSettings::NoGraphics,"NoGraphics");
		}

		template<> Reflection::EnumDesc<CRenderSettings::FrameRateManagerMode>::EnumDesc()
			:Reflection::EnumDescriptor("FramerateManagerMode")
		{
			addPair(CRenderSettings::FrameRateManagerAuto,"Automatic");
			addPair(CRenderSettings::FrameRateManagerOn,"On");
			addPair(CRenderSettings::FrameRateManagerOff,"Off");
		}

		template<> Reflection::EnumDesc<CRenderSettings::AntialiasingMode>::EnumDesc()
			:Reflection::EnumDescriptor("Antialiasing")
		{
			addPair(CRenderSettings::AntialiasingAuto,"Automatic");
			addPair(CRenderSettings::AntialiasingOff,"Off");
			addPair(CRenderSettings::AntialiasingOn,"On");
		}

		template<> Reflection::EnumDesc<CRenderSettings::QualityLevel>::EnumDesc()
		:Reflection::EnumDescriptor("QualityLevel")
		{
			addPair(CRenderSettings::QualityAuto, "Automatic");

			RBXASSERT(CRenderSettings::QualityLevelMax < 100);

			// Generating all of the names for quality levels inplace on one array
			for (int i = 1; i < CRenderSettings::QualityLevelMax; i++)
			{
				std::string name = RBX::format("Level%.2d", i);
				addPair((CRenderSettings::QualityLevel)i, name.c_str());
			}
			{
				char name [] = "Level 00";
				char* numberstart = &name[sizeof(name) - 3];
				for (int i = 1; i < CRenderSettings::QualityLevelMax; i++)
				{
					snprintf(numberstart, 3, "%2u", i);
					addLegacyName(name, (CRenderSettings::QualityLevel)i);
				}
			}
		}

		template<> Reflection::EnumDesc<CRenderSettings::ResolutionPreset>::EnumDesc()
		:Reflection::EnumDescriptor("Resolution")
		{
			addPair(CRenderSettings::ResolutionAuto, "Automatic");
			addPair(CRenderSettings::Resolution720x526,		"720x526");
			addPair(CRenderSettings::Resolution800x600,		"800x600");
			addPair(CRenderSettings::Resolution1024x600,	"1024x600");
			addLegacyName("1024x600 (wide)", CRenderSettings::Resolution1024x600);
			addPair(CRenderSettings::Resolution1024x768,	"1024x768");
			addPair(CRenderSettings::Resolution1280x720,	"1280x720");
			addLegacyName("1280x720 (wide)", CRenderSettings::Resolution1280x720);
			addPair(CRenderSettings::Resolution1280x768,	"1280x768");
			addLegacyName("1280x768 (wide)", CRenderSettings::Resolution1280x768);
			addPair(CRenderSettings::Resolution1152x864,	"1152x864");
			addPair(CRenderSettings::Resolution1280x800,	"1280x800");
			addLegacyName("1280x800 (wide)", CRenderSettings::Resolution1280x800);
			addPair(CRenderSettings::Resolution1360x768,	"1360x768");
			addLegacyName("1360x768 (wide)", CRenderSettings::Resolution1360x768);
			addPair(CRenderSettings::Resolution1280x960,	"1280x960");
			addPair(CRenderSettings::Resolution1280x1024,	"1280x1024");
			addPair(CRenderSettings::Resolution1440x900,	"1440x900");
			addLegacyName("1440x900 (wide)", CRenderSettings::Resolution1440x900);
			addPair(CRenderSettings::Resolution1600x900,	"1600x900");
			addLegacyName("1600x900 (wide)", CRenderSettings::Resolution1600x900);
			addPair(CRenderSettings::Resolution1600x1024,	"1600x1024");
			addLegacyName("1600x1024 (wide)", CRenderSettings::Resolution1600x1024);
			addPair(CRenderSettings::Resolution1600x1200,	"1600x1200");
			addPair(CRenderSettings::Resolution1680x1050,	"1680x1050");
			addLegacyName("1680x1050 (wide)", CRenderSettings::Resolution1680x1050);
			addPair(CRenderSettings::Resolution1920x1080,	"1920x1080");
			addLegacyName("1920x1080 (wide)", CRenderSettings::Resolution1920x1080);
			addPair(CRenderSettings::Resolution1920x1200,	"1920x1200");
			addLegacyName("1920x1200 (wide)", CRenderSettings::Resolution1920x1200);
		}
	}
}


REFLECTION_BEGIN();
static Reflection::EnumPropDescriptor<CRenderSettingsItem, CRenderSettingsItem::GraphicsMode> prop_graphicsMode("GraphicsMode", "General", &CRenderSettingsItem::getGraphicsMode, &CRenderSettingsItem::setGraphicsMode);
static Reflection::PropDescriptor<CRenderSettingsItem, bool> prop_exporterMergeByMaterial("ExportMergeByMaterial", "General", &CRenderSettingsItem::getObjExportMergeByMaterial, &CRenderSettingsItem::setObjExportMergeByMaterial);

static Reflection::EnumPropDescriptor<CRenderSettingsItem, CRenderSettingsItem::FrameRateManagerMode> prop_frameRateManagerMode("FrameRateManager", "General", &CRenderSettingsItem::getFrameRateManagerMode, &CRenderSettingsItem::setFrameRateManagerMode);
static Reflection::EnumPropDescriptor<CRenderSettingsItem, CRenderSettingsItem::QualityLevel> prop_frmQuality("QualityLevel", "Performance", &CRenderSettingsItem::getQualityLevel, &CRenderSettingsItem::setQualityLevel);
static Reflection::EnumPropDescriptor<CRenderSettingsItem, CRenderSettingsItem::QualityLevel> prop_frmEditQuality("EditQualityLevel", "Performance", &CRenderSettingsItem::getEditQualityLevel, &CRenderSettingsItem::setEditQualityLevel);

static Reflection::PropDescriptor<CRenderSettingsItem, bool> prop_ShowAggregation("IsAggregationShown", "Debug", &CRenderSettingsItem::getShowAggregation, &CRenderSettingsItem::setShowAggregation);

static Reflection::BoundProp<bool> prop_IsSynchronizedWithPhysics("IsSynchronizedWithPhysics", "Performance", &CRenderSettingsItem::isSynchronizedWithPhysics);
static Reflection::EnumPropDescriptor<CRenderSettingsItem, CRenderSettingsItem::AASamples> prop_AASamples("AASamples", "Quality", &CRenderSettingsItem::getAASamples, &CRenderSettingsItem::setAASamples);
Reflection::BoundProp<std::string> CRenderSettingsItem::prop_profileName("profileName", "Quality", &CRenderSettingsItem::profileName, Reflection::PropertyDescriptor::STREAMING);
static Reflection::EnumPropDescriptor<CRenderSettingsItem, CRenderSettingsItem::AntialiasingMode> prop_antialiasingMode("Antialiasing", "Quality", &CRenderSettingsItem::getAntialiasingMode, &CRenderSettingsItem::setAntialiasingMode);
static Reflection::PropDescriptor<CRenderSettingsItem, bool> prop_debugShowBoundingBoxes("ShowBoundingBoxes", "Debug", &CRenderSettingsItem::getDebugShowBoundingBoxes, &CRenderSettingsItem::setDebugShowBoundingBoxes);
static Reflection::PropDescriptor<CRenderSettingsItem, int> prop_autoQualityLevel("AutoFRMLevel", "Debug", &CRenderSettingsItem::getAutoQualityLevel, &CRenderSettingsItem::setAutoQualityLevel);

static Reflection::PropDescriptor<CRenderSettingsItem, bool> prop_enableFRM("EnableFRM", "Debug", &CRenderSettingsItem::getEnableFRM, &CRenderSettingsItem::setEnableFRM, Reflection::PropertyDescriptor::LEGACY_SCRIPTING);
static Reflection::PropDescriptor<CRenderSettingsItem, bool> prop_debugDisableInterpolation("DebugDisableInterpolation", "Debug", &CRenderSettingsItem::getDebugDisableInterpolation, &CRenderSettingsItem::setDebugDisableInterpolation);
static Reflection::PropDescriptor<CRenderSettingsItem, bool> prop_showInterpolationPath("ShowInterpolationpath", "Debug", &CRenderSettingsItem::getShowInterpolationPath, &CRenderSettingsItem::setShowInterpolationPath);

static Reflection::PropDescriptor<CRenderSettingsItem, bool> prop_debugReloadAssets("ReloadAssets", "Debug", &CRenderSettingsItem::getDebugReloadAssets, &CRenderSettingsItem::setDebugReloadAssets);

Reflection::EnumPropDescriptor<CRenderSettingsItem, CRenderSettingsItem::ResolutionPreset> CRenderSettingsItem::prop_resolution("Resolution", "Screen", &CRenderSettingsItem::getResolutionPreference, &CRenderSettingsItem::setResolutionPreference);

static Reflection::BoundFuncDesc<CRenderSettingsItem, int()> func_getMaxQualityLevel(&CRenderSettingsItem::getMaxQualityLevel, "GetMaxQualityLevel", Security::None);

static Reflection::PropDescriptor<CRenderSettingsItem, int> prop_textureCacheSize("TextureCacheSize", "Cache", &CRenderSettingsItem::getTextureCacheSize, &CRenderSettingsItem::setTextureCacheSize);
static Reflection::PropDescriptor<CRenderSettingsItem, int> prop_meshCacheSize("MeshCacheSize", "Cache", &CRenderSettingsItem::getMeshCacheSize, &CRenderSettingsItem::setMeshCacheSize);
REFLECTION_END();

const char* const sRenderSettings = "RenderSettings";

CRenderSettingsItem::CRenderSettingsItem()
	:isSynchronizedWithPhysics(false)
	, currentDisplaySize(800, 600)
{
	setName("Rendering");
	fullScreenSizes.push_back(currentDisplaySize); // at least one valid default.	

	size_t s = SystemUtil::getVideoMemory();
	if (s>=16000000)
		fullscreenSize = G3D::Vector2int16(1024, 768);
	else
		fullscreenSize = G3D::Vector2int16(800, 600);
}


void CRenderSettingsItem::setShowAggregation(bool value) {
	if (value!=showAggregation)
	{
		showAggregation = value;
		settingsChangedSignal(&prop_ShowAggregation);
	}
}

void CRenderSettingsItem::runProfiler(bool overwriteExistingValues)
{
	static const char* profileName = "profiled5";
	if (overwriteExistingValues || this->profileName!=profileName)
	{
		prop_profileName.setValue(this, profileName);
	}
}


void CRenderSettingsItem::setAASamples(AASamples value)
{
	if (value!=aaSamples)
	{
		aaSamples = value;
		settingsChangedSignal(&prop_AASamples);
	}
}

void CRenderSettingsItem::setResolutionPreference(ResolutionPreset value)
{
	if (value!=resolutionPreference)
	{
		resolutionPreference = value;
		settingsChangedSignal(&prop_resolution);
	}
}

void CRenderSettingsItem::setFullscreenSize(G3D::Vector2int16 value)
{
	fullscreenSize = value;
}

void CRenderSettingsItem::setWindowSize(G3D::Vector2int16 value)
{
	windowSize = value;
}

// Same pattern used in PhysicsSettings::30FPS Throttle
void CRenderSettingsItem::setGraphicsMode(GraphicsMode value) 
{
	if (value != graphicsMode) {
		graphicsMode = value;
		settingsChangedSignal(&prop_graphicsMode);
	}
}

#define SET_MODE(modeName, modeVar) \
void CRenderSettingsItem::set##modeName##Mode(modeName##Mode value) \
{\
	if (value != modeVar##Mode) {\
		modeVar##Mode = value;\
		settingsChangedSignal(&prop_##modeVar##Mode);\
	}\
}

SET_MODE(FrameRateManager,frameRateManager);
SET_MODE(Antialiasing,antialiasing);


void CRenderSettingsItem::setQualityLevel(QualityLevel value)
{
	if (value != qualityLevel) {
		qualityLevel = value;
		settingsChangedSignal(&prop_frmQuality);
	}
}
void CRenderSettingsItem::setEditQualityLevel(QualityLevel value)
{
	if (value != editQualityLevel) {
		editQualityLevel = value;
		settingsChangedSignal(&prop_frmEditQuality);
	}
}
void CRenderSettingsItem::setAutoQualityLevel(int value)
{
	if (value != autoQualityLevel) {
		autoQualityLevel = value;
		settingsChangedSignal(&prop_frmQuality);
	}
}

void CRenderSettingsItem::setDebugShowBoundingBoxes(bool value)
{
	if (value != debugShowBoundingBoxes) {
		debugShowBoundingBoxes = value;
		settingsChangedSignal(&prop_debugShowBoundingBoxes);
	}
}

void CRenderSettingsItem::setEnableFRM(bool value)
{
	if (value != enableFRM) {
		enableFRM = value;
		settingsChangedSignal(&prop_enableFRM);
	}
}

void CRenderSettingsItem::setTextureCacheSize(unsigned int size)
{
	textureCacheSize = size;
}
void CRenderSettingsItem::setMeshCacheSize(unsigned int size)
{
	meshCacheSize = size;
}

bool CRenderSettingsItem::getDebugDisableInterpolation() const
{
	return PartInstance::disableInterpolation;
}

void CRenderSettingsItem::setDebugDisableInterpolation(bool value)
{
	PartInstance::disableInterpolation = value;
}

bool CRenderSettingsItem::getShowInterpolationPath() const
{
	return PartInstance::showInterpolationPath;
}

void CRenderSettingsItem::setShowInterpolationPath(bool value)
{
	PartInstance::showInterpolationPath = value;
}

bool CRenderSettingsItem::getDebugReloadAssets() const
{
    return debugReloadAssets;
}

void CRenderSettingsItem::setDebugReloadAssets(bool value)
{
    debugReloadAssets = value;
}

bool CRenderSettingsItem::getObjExportMergeByMaterial() const
{
    return objExportMergeByMaterial;
}

void CRenderSettingsItem::setObjExportMergeByMaterial(bool value)
{
    objExportMergeByMaterial = value;
}

#define SET_VAR_NOREG(T, boolName, boolVar, Category) \
	void CRenderSettingsItem::set##boolName(T value) \
	{ \
		if (value != boolVar) { \
			boolVar = value; \
			settingsChangedSignal(&prop_##boolVar); \
		} \
	}

#define SET_VAR(T, boolName, boolVar, Category) \
	static Reflection::PropDescriptor<CRenderSettingsItem, T> prop_##boolVar(#boolName, Category, &CRenderSettingsItem::get##boolName, &CRenderSettingsItem::set##boolName); \
	SET_VAR_NOREG(T, boolName, boolVar, Category)

SET_VAR(bool, EagerBulkExecution, eagerBulkExecution, "Performance")

