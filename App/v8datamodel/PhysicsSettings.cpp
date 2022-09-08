/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/PhysicsSettings.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/PartOperation.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/DataModel.h"
#include "V8World/World.h"
#include "V8World/Primitive.h"
#include "V8World/Buoyancy.h"
#include "Util/RunStateOwner.h"
#include "V8datamodel/FastLogSettings.h"

const char *const RBX::sPhysicsSettings = "PhysicsSettings";

// Don't show tree in release mode - has ROBLOX specific secrets as to how we do assembly

#ifdef _DEBUG
	#define __RBX_SECRET_DEBUGGING
#endif

#ifdef _NOOPT
	#define __RBX_SECRET_DEBUGGING
#endif

using namespace RBX;

REFLECTION_BEGIN();
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ShowAnchoredParts("AreAnchorsShown", "Display", &PhysicsSettings::getShowAnchoredParts, &PhysicsSettings::setShowAnchoredParts);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ShowPartCoordinateFrames("ArePartCoordsShown", "Display", &PhysicsSettings::getShowPartCoordinateFrames, &PhysicsSettings::setShowPartCoordinateFrames);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ShowUnalignedParts("AreUnalignedPartsShown", "Display", &PhysicsSettings::getShowUnalignedParts, &PhysicsSettings::setShowUnalignedParts);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ShowModelCoordinateFrames("AreModelCoordsShown", "Display", &PhysicsSettings::getShowModelCoordinateFrames, &PhysicsSettings::setShowModelCoordinateFrames);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ShowWorldCoordinateFrame("AreWorldCoordsShown", "Display", &PhysicsSettings::getShowWorldCoordinateFrame, &PhysicsSettings::setShowWorldCoordinateFrame);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ShowEPhysicsOwners("AreOwnersShown", "Display", &PhysicsSettings::getShowEPhysicsOwners, &PhysicsSettings::setShowEPhysicsOwners);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ShowEPhysicsRegions("AreRegionsShown", "Display", &PhysicsSettings::getShowEPhysicsRegions, &PhysicsSettings::setShowEPhysicsRegions);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_HighlightAwakeParts("AreAwakePartsHighlighted", "Display", &PhysicsSettings::getHighlightAwakeParts, &PhysicsSettings::setHighlightAwakeParts);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ShowBodyTypes("AreBodyTypesShown", "Display", &PhysicsSettings::getShowBodyTypes, &PhysicsSettings::setShowBodyTypes);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ShowReceiveAge("IsReceiveAgeShown", "Display", &PhysicsSettings::getShowReceiveAge, &PhysicsSettings::setShowReceiveAge);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ShowContactPoints("AreContactPointsShown", "Display", &PhysicsSettings::getShowContactPoints, &PhysicsSettings::setShowContactPoints);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ShowJointCoordinates("AreJointCoordinatesShown", "Display", &PhysicsSettings::getShowJointCoordinates, &PhysicsSettings::setShowJointCoordinates);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ShowMechanisms("AreMechanismsShown", "Display", &PhysicsSettings::getShowMechanisms, &PhysicsSettings::setShowMechanisms);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ShowAssemblies("AreAssembliesShown", "Display", &PhysicsSettings::getShowAssemblies, &PhysicsSettings::setShowAssemblies);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ShowSpanningTree("IsTreeShown", "Display", &PhysicsSettings::getShowSpanningTree, &PhysicsSettings::setShowSpanningTree);

static Reflection::PropDescriptor<PhysicsSettings, bool> prop_AllowSleep("AllowSleep", "Performance", &PhysicsSettings::getAllowSleep, &PhysicsSettings::setAllowSleep);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ParallelPhysics("ParallelPhysics", "Performance", &PhysicsSettings::getParallelPhysics, &PhysicsSettings::setParallelPhysics);
static Reflection::EnumPropDescriptor<PhysicsSettings, EThrottle::EThrottleType> prop_enviromentalThrottle("PhysicsEnvironmentalThrottle", "Performance", &PhysicsSettings::getEThrottle, &PhysicsSettings::setEThrottle);
static Reflection::PropDescriptor<PhysicsSettings, double> prop_ThrottleAdjustTime("ThrottleAdjustTime", "Performance", &PhysicsSettings::getThrottleAdjustTime, &PhysicsSettings::setThrottleAdjustTime);

static Reflection::PropDescriptor<PhysicsSettings, bool> prop_RenderDecompositionData("ShowDecompositionGeometry", "Display", &PhysicsSettings::getRenderDecompositionData, &PhysicsSettings::setRenderDecompositionData);
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_PhysicsAnalyzerEnabled("PhysicsAnalyzerEnabled", category_Data, &PhysicsSettings::getPhysicsAnalyzerState, NULL, Reflection::PropertyDescriptor::UI, Security::Plugin);

#if defined(RBX_TEST_BUILD) || RBX_PLATFORM_IOS
static Reflection::PropDescriptor<PhysicsSettings, bool> prop_ThrottleAt30Fps("Is30FpsThrottleEnabled", "Performance", &PhysicsSettings::getThrottleAt30Fps, &PhysicsSettings::setThrottleAt30Fps);
#endif
REFLECTION_END();

PhysicsSettings::PhysicsSettings() : throttleAdjustTime(1.0), physicsAnalyzerState(false)
{
	setName("Physics");
}


#define SET_GET(Type, PropName, ObjName, propVar) \
\
\
void PhysicsSettings::set##PropName(Type value) {\
	if (value != get##PropName())	{\
		ObjName::propVar = value;\
		raiseChanged(prop_##PropName);\
	}\
}\
Type PhysicsSettings::get##PropName() const {\
	return (ObjName::propVar);\
}\

// Display
SET_GET(bool, ShowAnchoredParts, PartInstance, showAnchoredParts);
SET_GET(bool, ShowPartCoordinateFrames, PartInstance, showPartCoordinateFrames);
SET_GET(bool, ShowUnalignedParts, PartInstance, showUnalignedParts);
SET_GET(bool, ShowModelCoordinateFrames, ModelInstance, showModelCoordinateFrames);
SET_GET(bool, ShowWorldCoordinateFrame, Workspace, showWorldCoordinateFrame);		// TODO: Refactor: Take this field out of Workspace
SET_GET(bool, ShowEPhysicsOwners, Workspace, showEPhysicsOwners);
SET_GET(bool, ShowEPhysicsRegions, Workspace, showEPhysicsRegions);
SET_GET(bool, HighlightAwakeParts, PartInstance, highlightAwakeParts);
SET_GET(bool, ShowBodyTypes, PartInstance, showBodyTypes);
SET_GET(bool, ShowReceiveAge, PartInstance, showReceiveAge);
SET_GET(bool, ShowContactPoints, PartInstance, showContactPoints);
SET_GET(bool, ShowJointCoordinates, PartInstance, showJointCoordinates);
SET_GET(bool, ShowMechanisms, PartInstance, showMechanisms);
SET_GET(bool, ShowAssemblies, PartInstance, showAssemblies);
SET_GET(bool, ShowSpanningTree, PartInstance, showSpanningTree);

// Performance
SET_GET(bool, AllowSleep, Primitive, allowSleep);
SET_GET(bool, ParallelPhysics, RunService, parallelPhysicsUserEnabled);

// CSG Visibility
//SET_GET(bool, RenderDecompositionData, PartOperation, renderCollisionData);

void PhysicsSettings::setRenderDecompositionData(bool value)
{
	if (value != getRenderDecompositionData())
	{
		PartOperation::renderCollisionData = value;
		raisePropertyChanged(prop_RenderDecompositionData);
	}
}

bool PhysicsSettings::getRenderDecompositionData() const
{
	return PartOperation::renderCollisionData;
}


EThrottle::EThrottleType PhysicsSettings::getEThrottle() const
{
	return EThrottle::globalDebugEThrottle;
}

void PhysicsSettings::setEThrottle(EThrottle::EThrottleType value) 
{
	if (value != getEThrottle()) {
		EThrottle::globalDebugEThrottle = value;
		raiseChanged(prop_enviromentalThrottle);
	}
}

bool PhysicsSettings::getThrottleAt30Fps() const
{
	return DataModel::throttleAt30Fps;
}

void PhysicsSettings::setThrottleAt30Fps(bool value) 
{
	// Task Scheduler also needs to know about the current state of the DataModel Throttle.
	if (TaskScheduler::singleton().isCyclicExecutive() && TaskScheduler::singleton().DataModel30fpsThrottle != value)
	{
		TaskScheduler::singleton().DataModel30fpsThrottle = value;
	}
	if (value != getThrottleAt30Fps()) {
		DataModel::throttleAt30Fps = value;
#ifdef MEMORY_PROFILE
		raiseChanged(prop_ThrottleAt30Fps);
#endif
	}
}

void PhysicsSettings::setThrottleAdjustTime(double value)
{
	if (value != throttleAdjustTime)
	{
		throttleAdjustTime = value;
		raiseChanged(prop_ThrottleAdjustTime);
	}
}

void PhysicsSettings::setPhysicsAnalyzerState( bool enabled )
{
    if (physicsAnalyzerState != enabled)
    {
        physicsAnalyzerState = enabled;
        raisePropertyChanged(prop_PhysicsAnalyzerEnabled);
    }
}


#ifdef __RBX_SECRET_DEBUGGING

    REFLECTION_BEGIN();
	static Reflection::PropDescriptor<PhysicsSettings, bool> prop_HighlightSleepParts("AreSleepPartsHighlighted", "Secret Display", &PhysicsSettings::getHighlightSleepParts, &PhysicsSettings::setHighlightSleepParts);
	static Reflection::PropDescriptor<PhysicsSettings, float> prop_WaterViscosity("WaterViscosity", "Simulation", &PhysicsSettings::getWaterViscosity, &PhysicsSettings::setWaterViscosity);
    REFLECTION_END();
    
SET_GET(bool, HighlightSleepParts, PartInstance, highlightSleepParts);
SET_GET(float, WaterViscosity, BuoyancyContact, waterViscosity);
#endif

