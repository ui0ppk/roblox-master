/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/PartOperation.h"
#include "V8World/World.h"
#include "V8World/Contact.h"
#include "V8World/Joint.h"
#include "V8World/WeldJoint.h"
#include "V8World/ContactManagerSpatialHash.h"
#include "V8World/ContactManager.h"
#include "V8World/SendPhysics.h"
#include "V8World/Mechanism.h"

#include "V8World/CleanStage.h"
#include "V8World/GroundStage.h"
#include "V8World/TreeStage.h"
#include "V8World/SpatialFilter.h"
#include "V8World/AssemblyStage.h"
#include "V8World/MovingAssemblyStage.h"
#include "V8World/StepJointsStage.h"
#include "V8World/HumanoidStage.h"
#include "V8World/SleepStage.h"
#include "V8World/SimulateStage.h"

#include "V8World/JointBuilder.h"
#include "V8World/Assembly.h"
#include "V8World/Clump.h"
#include "V8Kernel/Kernel.h"
#include "V8Kernel/Constants.h"
#include "v8kernel/Body.h"
#include "Util/Profiling.h"
#include "Util/Analytics.h"
#include "Network/NetworkOwner.h"

#include "v8datamodel/JointInstance.h"
#include "v8datamodel/MegaCluster.h"

#include "BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "BulletCollision/GImpact/btGImpactCollisionAlgorithm.h"

#include "rbx/Profiler.h"

LOGVARIABLE(CyclicExecutiveWorldSteps, 0)
LOGGROUP(Physics)


DYNAMIC_FASTINTVARIABLE(BulletContactBreakThresholdPercent, 200)
DYNAMIC_FASTINTVARIABLE(BulletContactBreakOrthogonalThresholdPercent, 200)
DYNAMIC_FASTINTVARIABLE(BulletContactBreakOrthogonalThresholdActivatePercent, 200)
DYNAMIC_FASTINTVARIABLE(WorldStepMax,30)
DYNAMIC_FASTINTVARIABLE(WorldStepsOffsetAdjustRate,100) /* in thousands */
DYNAMIC_FASTINTVARIABLE(smoothnessReportThreshold,10000)

DYNAMIC_FASTINTVARIABLE(MaxMissedWorldStepsRemembered, 12)
DYNAMIC_FASTFLAGVARIABLE(CyclicExecutiveThrottlingCancelWorldStepAccum, false)
DYNAMIC_FASTFLAG(MaterialPropertiesEnabled)

FASTINTVARIABLE(PhysicsBulletManifoldPoolSize, 1024)
SYNCHRONIZED_FASTFLAG(MaterialPropertiesNewIsDefault)
SYNCHRONIZED_FASTFLAG(NewPhysicalPropertiesForcedOnAll)

FASTFLAGVARIABLE(FixBulletGJKOptimization, false)
FASTFLAGVARIABLE(FixBulletGJKOptimization2, false)

FASTFLAG(PhysicsAnalyzerEnabled)
LOGVARIABLE(WorldStepsBehind, 0)
LOGVARIABLE(WorldStepsBehindG, 0)

extern bool gUseTriangleNormalForConvexTriangle;
extern bool gFixBulletGJKOptimization;
extern bool gFixBulletGJKOptimization2;

namespace RBX {

EThrottle::EThrottleType EThrottle::globalDebugEThrottle = EThrottle::ThrottleDefaultAuto;	// globalSwitch

// Corresponds to these throttling ratios (for A/B, this is read as A steps throttled for every B Steps)
// 0/8, 1/8, 1/4, 1/3, 1/2, 2/3, 3/4, 7/8, 15/16
int const EThrottle::throttleSetting[] = {100000000, 8, 4, 3, 2, -3, -4, -8, -16};

EThrottle::EThrottle()
: requestedSkip(1)
, throttleIndex(1)
{
	if(globalDebugEThrottle == EThrottle::ThrottleDefaultAuto)
		usedSkip = 100000000;
	else
		usedSkip = 1;
}

bool EThrottle::increaseLoad(bool increase) 
{
	if(globalDebugEThrottle == EThrottle::ThrottleDefaultAuto)
	{
		if (increase) {
			if (throttleIndex > 0) {
				throttleIndex -= 1;
				return true;
			}
		}
		else {
			if (throttleIndex < 8) {
				throttleIndex += 1;
				return true;
			}
		}
	}
	else
	{
		if (increase) {
			if (requestedSkip > 1) {
				requestedSkip /= 2;
				return true;
			}
		}
		else {
			if (requestedSkip < 16) {
				requestedSkip *= 2;
				return true;
			}
		}
	}

	return false;
}


bool EThrottle::computeThrottle(int step)
{
	switch (globalDebugEThrottle)
	{
		case EThrottle::ThrottleDefaultAuto:
			{
				usedSkip = throttleSetting[throttleIndex];
				return ((usedSkip > 0) == ((step % usedSkip) == 0));
			}
		case EThrottle::Skip2:
			{
				usedSkip = 2;
				return ((step % 2) != 0);	
			}
		case EThrottle::Skip4:
			{
				usedSkip = 4;
				return ((step % 4) != 0);	
			}
		case EThrottle::Skip8:
			{
				usedSkip = 8;
				return ((step % 8) != 0);	
			}
		case EThrottle::Skip16:
			{
				usedSkip = 16;
				return ((step % 16) != 0);	
			}
		case EThrottle::ThrottleAlways:
			{
				usedSkip = 100000000;
				return true;
			}
		case EThrottle::ThrottleDisabled:
		default:
			{
				usedSkip = 1;
				return false;
			}
	}
}

float EThrottle::getEnvironmentSpeed() const
{
	if(globalDebugEThrottle == EThrottle::ThrottleDefaultAuto) 
		return usedSkip > 0 ? (1.0f - (1.0f / usedSkip)) : -1.0f / usedSkip;
	else
		return 1.0f / usedSkip;
}

////////////////////////////////////////////////////////////////////////////////////

World::World() :
#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
	contactManager(new ContactManager(this)),
	cleanStage(new CleanStage(NULL, this)),
#pragma warning(pop)
	sendPhysics( new SendPhysics() ),
	worldSteps(0),
	worldStepId(0),
	worldStepAccumulated(4.0f),         //Initialize 4 step so that we can run WorldSteps 240Hz consistently on the first run
	inStepCode(false),
	inJointNotification(NULL),
	numContacts(0),
	numJoints(0),
	numLinkCalls(0),
	groundPrimitive( new Primitive(Geometry::GEOMETRY_BLOCK) ),
	profilingWorldStep(new Profiling::CodeProfiler("World Step")),
	profilingBreak(new Profiling::CodeProfiler("Break Time")),
	profilingAssembly(new Profiling::CodeProfiler("Assembler Time")),
	profilingFilter(new Profiling::CodeProfiler("Filter Time")),
	profilingUiStep(new Profiling::CodeProfiler("UI Step")),
	frmThrottle(1.0f),
	bulletDispatcher(NULL),
	bulletCollisionConfiguration(NULL),
    UIDGenerator( 0 ),
    physicsAnalyzerEnabled( false ),
	fallenPartDestroyHeight(-500.0f),
	physicalMaterialsMode(PhysicalPropertiesMode_Default),
	errorCount( 0.0),
	passCount(0.0),
    lastNumWorldSteps(0),
	lastSendTimeStamp(Time::nowFast()),
	lastFrameTimeStamp(Time::nowFast()),
	worldStepOffset(0.0),
	frameinfosSize(0.0),
	frameinfosTarget(0.0),
	targetDelayTenths(0.0),
	infosSizeTenths(0.0),
	maxDelta(0.0),
	frameinfosCount(0.0)

{
	groundPrimitive->setAnchoredProperty(true);
	groundPrimitive->setWorld(this);
	cleanStage->onPrimitiveAdded(groundPrimitive);

	std::vector<RBX::Profiling::CodeProfiler*> worldProfilers;
	loadProfilers(worldProfilers);

	// set up collision world, collision configs, etc
    btDefaultCollisionConstructionInfo info;
    info.m_defaultMaxPersistentManifoldPoolSize = FInt::PhysicsBulletManifoldPoolSize;

	bulletCollisionConfiguration = new btDefaultCollisionConfiguration(info);
	bulletDispatcher = new btCollisionDispatcher(bulletCollisionConfiguration);	
	FASTLOG1(FLog::Physics, "Set bulletDispatcher to: %p", bulletDispatcher);
	btGImpactCollisionAlgorithm::registerAlgorithm(bulletDispatcher);

	// This fixes Bullet collision normal between convexes and triangle meshes to be exactly the same as triangle normal
    gUseTriangleNormalForConvexTriangle = true;

	if (FFlag::FixBulletGJKOptimization)
	{
		gFixBulletGJKOptimization = true;
	}
	if (FFlag::FixBulletGJKOptimization2)
	{
		gFixBulletGJKOptimization2 = true;
	}
}

World::~World() 
{
	cleanStage->onPrimitiveRemoving(groundPrimitive);
	RBXASSERT(groundPrimitive->getNumEdges() == 0);
	groundPrimitive->setWorld(NULL);

	RBXASSERT(numJoints == 0);
	RBXASSERT(numContacts == 0);
	RBXASSERT(primitives.size() == 0);
	RBXASSERT(breakableJoints.empty());

	delete groundPrimitive;
	delete sendPhysics;
	delete cleanStage;
	delete contactManager;

	if (bulletDispatcher)
		delete bulletDispatcher;
	if (bulletCollisionConfiguration)
		delete bulletCollisionConfiguration;
	FASTLOG1(FLog::Physics, "Remove bulletDispatcher to: %p", bulletDispatcher);
}


void World::loadProfilers(std::vector<RBX::Profiling::CodeProfiler*>& worldProfilers) const
{
	worldProfilers.push_back(profilingBreak.get());
	worldProfilers.push_back(profilingAssembly.get());
	worldProfilers.push_back(profilingFilter.get());
	worldProfilers.push_back(profilingUiStep.get());
	worldProfilers.push_back(getContactManager()->profilingBroadphase.get());
	worldProfilers.push_back(getSleepStage()->profilingCollision.get());
	worldProfilers.push_back(getSleepStage()->profilingJointSleep.get());
	worldProfilers.push_back(getSleepStage()->profilingWake.get());
	worldProfilers.push_back(getSleepStage()->profilingSleep.get());
	worldProfilers.push_back(getStepJointsStage()->profilingJointUpdate.get());
	worldProfilers.push_back(getKernel()->profilingKernelBodies.get());
	worldProfilers.push_back(getKernel()->profilingKernelConnectors.get());
}


TreeStage* World::getTreeStage()
{
	return rbx_static_cast<TreeStage*>(cleanStage->findStage(IStage::TREE_STAGE));
}



SpatialFilter* World::getSpatialFilter()
{
	return rbx_static_cast<SpatialFilter*>(cleanStage->findStage(IStage::SPATIAL_FILTER));
}

const SpatialFilter* World::getSpatialFilter() const
{
	return rbx_static_cast<SpatialFilter*>(cleanStage->findStage(IStage::SPATIAL_FILTER));
}



AssemblyStage* World::getAssemblyStage()
{
	return rbx_static_cast<AssemblyStage*>(cleanStage->findStage(IStage::ASSEMBLY_STAGE));
}

const AssemblyStage* World::getAssemblyStage() const
{
	return rbx_static_cast<AssemblyStage*>(cleanStage->findStage(IStage::ASSEMBLY_STAGE));
}



MovingAssemblyStage* World::getMovingAssemblyStage()
{
	return rbx_static_cast<MovingAssemblyStage*>(cleanStage->findStage(IStage::MOVING_ASSEMBLY_STAGE));
}




StepJointsStage* World::getStepJointsStage()
{
	return rbx_static_cast<StepJointsStage*>(cleanStage->findStage(IStage::STEP_JOINTS_STAGE));
}


const StepJointsStage* World::getStepJointsStage() const
{
	return rbx_static_cast<StepJointsStage*>(cleanStage->findStage(IStage::STEP_JOINTS_STAGE));
}

GroundStage* World::getGroundStage()
{
	return rbx_static_cast<GroundStage*>(cleanStage->findStage(IStage::GROUND_STAGE));
}

HumanoidStage* World::getHumanoidStage() 
{
	return rbx_static_cast<HumanoidStage*>(cleanStage->findStage(IStage::HUMANOID_STAGE));
}

SleepStage* World::getSleepStage()
{
	return rbx_static_cast<SleepStage*>(cleanStage->findStage(IStage::SLEEP_STAGE));
}

const SleepStage* World::getSleepStage() const
{
	return rbx_static_cast<SleepStage*>(cleanStage->findStage(IStage::SLEEP_STAGE));
}



SimulateStage* World::getSimulateStage()
{
	return rbx_static_cast<SimulateStage*>(cleanStage->findStage(IStage::SIMULATE_STAGE));
}


const Kernel* World::getKernel() const
{
	return cleanStage->getKernel();
}

Kernel*	World::getKernel()
{
	return cleanStage->getKernel();
}

SendPhysics* World::getSendPhysics()
{
	return sendPhysics;
}

SimSendFilter& World::getSimSendFilter()
{
	return getSpatialFilter()->getSimSendFilter();
}


int World::getNumBodies() const
{
	return getKernel()->numBodies();
}

int World::getNumPoints() const
{
	return getKernel()->numPoints();
}

int World::getNumConstraints() const
{
	return getKernel()->numConnectors();
}


int World::getMetric(IWorldStage::MetricType metricType) const
{
	return cleanStage->getMetric(metricType);
}

int World::getNumHashNodes() const
{
	return contactManager->getSpatialHash()->getNodesOut();
}

int World::getMaxBucketSize() const
{
	return contactManager->getSpatialHash()->getMaxBucket();
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//
//  onChanged Functions
//

void World::ticklePrimitive(Primitive* p, bool recursive)
{
	if (!p) 
		return;

	RBXASSERT(p->getWorld());
	Assembly* a = p->getAssembly();
	if (a) {
		getSleepStage()->onExternalTickleAssembly(a, recursive);
	}
}

Assembly* World::onPrimitiveEngineChanging(Primitive* p)
{
	RBXASSERT(p->getWorld());
	RBXASSERT(p->getAssembly());
	RBXASSERT(p->inPipeline());
	RBXASSERT(!inStepCode);

	return getAssemblyStage()->onEngineChanging(p);
}

void World::onPrimitiveEngineChanged(Assembly* changing)
{
	RBXASSERT(changing);
	getAssemblyStage()->onEngineChanged(changing);
}


void World::onPrimitiveFixedChanging(Primitive* p)
{
	RBXASSERT(p->getWorld());
	RBXASSERT(!inStepCode);
	getGroundStage()->onPrimitiveFixedChanging(p);
}	


void World::onPrimitiveFixedChanged(Primitive* p)
{
	RBXASSERT(p->getWorld());
	RBXASSERT(!inStepCode);
	getGroundStage()->onPrimitiveFixedChanged(p);
}	

void World::onPrimitivePreventCollideChanged(Primitive* p)
{
	ticklePrimitive(p, false);
	RBXASSERT(p->getWorld());
	Contact* c = p->getFirstContact();
	while (c) {
		Primitive* other = c->otherPrimitive(p);
		ticklePrimitive(other, false);
		c = p->getNextContact(c);
	}
}

void World::onPrimitiveContactParametersChanged(Primitive* p)
{
	RBXASSERT(p->getWorld());
	Contact* c = p->getFirstContact();
	while (c) {
		c->onPrimitiveContactParametersChanged();
		c = p->getNextContact(c);
	}
}

void World::onPrimitiveExtentsChanged(Primitive* p)
{
	RBXASSERT(p->getWorld());
	RBXASSERT(!inStepCode);
	contactManager->onPrimitiveExtentsChanged(p);
}

void World::onAssemblyInSimluationStage(Assembly* a)
{
	getMovingAssemblyStage()->removeMovingGroundedAssembly(a);
}

void World::onPrimitiveGeometryChanged(Primitive* p)
{
	RBXASSERT(p->getWorld());
	RBXASSERT(!inStepCode);
	contactManager->onPrimitiveGeometryChanged(p);
}

void World::onJointPrimitiveNulling(Joint* j, Primitive* p)
{
	cleanStage->onJointPrimitiveNulling(j, p);
}

void World::onJointPrimitiveSet(Joint* j, Primitive* p)
{
	cleanStage->onJointPrimitiveSet(j, p);
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

void World::assemble()
{
    RBXPROFILER_SCOPE("Physics", "assemble");

	getTreeStage()->assemble();
}

bool World::isAssembled() 
{
	return getTreeStage()->isAssembled();
}

void World::setFRMThrottle(int value)
{
	frmThrottle = value;
}

void World::sendClumpChangedMessage(Primitive* childPrim)
{
	getTreeStage()->sendClumpChangedMessage(childPrim);
}

static void NotifyMovingRef(Assembly& assembly)
{
	assembly.notifyMovedFromInternalPhysics();
}

static void NotifyMoving(Assembly* assembly) 
{
	assembly->notifyMovedFromInternalPhysics();
}

template<class Set>
void notifyMovingPrimitives(const Set& assemblies)
{
	for_each(assemblies.begin(), assemblies.end(), NotifyMoving);
}

void World::notifyMovingAssemblies()
{
    RBXPROFILER_SCOPE("Physics", "notifyMovingAssemblies");

	RBX::Profiling::Mark mark(*profilingAssembly, false);
	//StandardOut::singleton()->printf(MESSAGE_INFO, "MovingDynamic %d, Humanoid %d, MovingGrounded %d", getSimulateStage()->getMovingDynamicAssembliesSize(), getHumanoidStage()->getMovingHumanoidAssemblies().size(), getMovingAssemblyStage()->getMovingGroundedAssembliesSize());
	std::for_each(getSimulateStage()->getRealTimeAssembliesBegin(), getSimulateStage()->getRealtimeAssembliesEnd(), NotifyMovingRef);
	std::for_each(getSimulateStage()->getMovingDynamicAssembliesBegin(), getSimulateStage()->getMovingDynamicAssembliesEnd(), NotifyMovingRef);
	notifyMovingPrimitives( getMovingAssemblyStage()->getMovingGroundedAssemblies() );
	notifyMovingPrimitives( getMovingAssemblyStage()->getMovingAnimatedAssemblies() );
	notifyMovingPrimitives( getHumanoidStage()->getMovingHumanoidAssemblies() );		
}

void World::uiStep(bool longStep, double distributedGameTime)
{
	RBXPROFILER_SCOPE("Physics", "uiStep");

	RBXASSERT(isAssembled());				// testing - assemble in 

	RBXASSERT(!inStepCode);

	if (longStep)
	{
		RBX::Profiling::Mark mark(*profilingBreak, false);
		doBreakJoints();		// out of step code here
	}

	inStepCode = true;

	if (longStep)
	{
		RBX::Profiling::Mark mark(*profilingAssembly, false);
		assemble();
	}

	{
		RBX::Profiling::Mark mark(*profilingFilter, false);
		getSpatialFilter()->filterStep();
	}

	{
		RBX::Profiling::Mark mark(*profilingUiStep, false);
		getMovingAssemblyStage()->jointsStepUi(distributedGameTime);												// update joints
	}

    inStepCode = false;
	RBXASSERT(isAssembled());
}


// 240 Hz
void World::doWorldStep(bool throttling, int uiStepId, int numThreads, boost::uint64_t debugTime)
{
	RBXPROFILER_SCOPE("Physics", "doWorldStep");

	inStepCode = true;

	RBXASSERT(isAssembled());																// Assembly - Tree Stage

    if( FFlag::PhysicsAnalyzerEnabled && getKernel()->getUsingPGSSolver() )
    {
        getKernel()->pgsSolver.setInconsistentConstraintDetectorEnabled( physicsAnalyzerEnabled );
        if( physicsAnalyzerEnabled && getKernel()->pgsSolver.getPhysicsAnalyzerBreakOnIssue() )
        {
            // Stop updating if there are inconsistencies
            if( getKernel()->pgsSolver.getInconsistentBodies().size() > 0 )
            {
                inStepCode = false;
                return;
            }
        }
    }

	getSleepStage()->stepSleepStage(worldStepId, uiStepId, throttling);	// update contacts, sleeping, awake

	getStepJointsStage()->jointsStepWorld();												// update joints

	getKernel()->step(throttling, numThreads, debugTime);				// solve

	inStepCode = false;
	
	{
        RBXPROFILER_SCOPE("Physics", "updateBroadphase");

		RBX::Profiling::Mark mark(*contactManager->profilingBroadphase, false);
		std::for_each(
			getSimulateStage()->getRealTimeAssembliesBegin(), 
			getSimulateStage()->getRealtimeAssembliesEnd(),
			boost::bind(&ContactManager::onAssemblyMovedFromStep, contactManager, _1) );

		if (!throttling)
		{
			std::for_each(
				getSimulateStage()->getMovingDynamicAssembliesBegin(), 
				getSimulateStage()->getMovingDynamicAssembliesEnd(),
				boost::bind(&ContactManager::onAssemblyMovedFromStep, contactManager, _1) );		// old Pre_step is here - updates Extents
		}
	}

	RBXASSERT(isAssembled());																// Assembly - Tree Stage
}

int World::getUiStepId()
{	
	return worldStepId / Constants::worldStepsPerUiStep();
}

int World::getLongUiStepId()
{	
	return worldStepId / Constants::worldStepsPerLongUiStep();
}

float World::getUpdateExpectedStepDelta()
{
	
	Time::Interval deltaTime;
	int  mdws = getWorldStepId();


	if (lastNumWorldSteps != 0)
	{
		double deltaWorldSteps		= mdws - lastNumWorldSteps; // constant
		deltaTime					= lastFrameTimeStamp - lastSendTimeStamp ;  // constant
		double expectedWorldSteps	=  (deltaTime.seconds() * Constants::worldStepsPerSec());  //constant
		worldStepOffset				+= expectedWorldSteps - deltaWorldSteps;
		worldStepOffset				= std::max((double)-DFInt::WorldStepMax ,std::min(worldStepOffset, (double)DFInt::WorldStepMax));

		// now drift toward 0;
		double adjust = (double)(DFInt::WorldStepsOffsetAdjustRate)/1000.0;
		if(worldStepOffset > adjust)
			worldStepOffset -= adjust;
		else if(worldStepOffset < -adjust)
			worldStepOffset += adjust;
		else
			worldStepOffset = 0.0;

		FASTLOG1F(FLog::WorldStepsBehind, "DELTA     WorldSteps       : %d", (float)deltaWorldSteps);
		FASTLOG1F(FLog::WorldStepsBehind, "Expected  WorldSteps       : %d", (float)expectedWorldSteps);
		FASTLOG1F(FLog::WorldStepsBehind,  "dt: %f", (float)(deltaTime.seconds()));
		FASTLOG3F(FLog::WorldStepsBehindG, "Number of WorldSteps behind, wsps, deltaTime: %f  %f  %f", (float)worldStepOffset, (float)deltaWorldSteps, (float)expectedWorldSteps);
	}

	if(mdws != getWorldStepId())
	{
		StandardOut::singleton()->printf(MESSAGE_INFO, "getWorldStepId changed %d %d", mdws, getWorldStepId());
	}

	lastNumWorldSteps     = getWorldStepId();
	lastSendTimeStamp     = lastFrameTimeStamp;

	return worldStepOffset / Constants::worldStepsPerSec();
}

int World::updateStepsRequiredForCyclicExecutive(float desiredInterval)
{
    if (!DataModel::throttleAt30Fps)
    {
        worldSteps = 8;
        return worldSteps;
    }

	float worldStepsFloat = desiredInterval * (float)Constants::worldStepsPerSec() + worldStepAccumulated;
	float worldStepsFloor = ::floorf( worldStepsFloat );
	float maxWorldStepsAllowedPerDMStep;
	if (DFFlag::CyclicExecutiveThrottlingCancelWorldStepAccum)
	{
		// When under heavy throttling where each WorldStep takes a long time,
		// we can get into a state where ROBLOX wants to run 15 physics WorldSteps in 1 frame,
		// and even with max throttling we will still have to run at least 2, which could potentially
		// hinder Rendering framerate
		maxWorldStepsAllowedPerDMStep = (eThrottle.getEnvironmentSpeed() > 0.4f) ? (float)DFInt::MaxMissedWorldStepsRemembered : 4.0f;
	}
	else
	{
		maxWorldStepsAllowedPerDMStep = (float)DFInt::MaxMissedWorldStepsRemembered;
	}

	worldStepsFloor = std::min( worldStepsFloor, maxWorldStepsAllowedPerDMStep);				// For perf reasons clamp max steps to 1/20th of a second at 240hz

	worldStepAccumulated = worldStepsFloat - worldStepsFloor;
	worldStepAccumulated = std::min( worldStepAccumulated, 1.99999f );

	// Drop remaining steps in response to spikes.  Even allowing this much accumulation
	// results allows for intervals where as high as 62.99s simulation has occurred.
	if( worldStepsFloor <= 0.0f)
	{
		return 0.0f;
	}

	worldSteps = (int)worldStepsFloor;
	FASTLOG1(FLog::CyclicExecutiveWorldSteps, "World Steps this Iteration: %d", worldSteps);
	return worldSteps;
}

float World::step(bool longStep, double distributedGameTime, float desiredInterval, int numThreads) 
{
	RBXASSERT(!inStepCode);
	RBXASSERT(isAssembled());				// testing - assemble in 


	// Update bullet Extern Globals with Dynamic FastInts
	gContactBreakingThreshold = DFInt::BulletContactBreakThresholdPercent/100.0 * 0.02;
	gContactThresholdOrthogonalFactor = DFInt::BulletContactBreakOrthogonalThresholdPercent/100.0;
	gContactThresholdOrthogonalActivateFactor = DFInt::BulletContactBreakOrthogonalThresholdActivatePercent/100.0;

	lastFrameTimeStamp = TaskScheduler::singleton().lastCyclcTimestamp;

	//
// On Mac there is nothing equivalent of _CrtCheckMemory to validate the state of the heap
// Instead use this on XCode http://developer.apple.com/library/ios/#documentation/Performance/Conceptual/ManagingMemory/Articles/MallocDebug.html
#ifdef _WIN32
	RBXASSERT_IF_VALIDATING(_CrtCheckMemory() != 0);
#endif

//	RBXASSERT(desiredInterval > 0.01);	// less than 100 FPS
//	RBXASSERT(desiredInterval < 0.10); // more than 10 FPS
//	RBXASSERT((worldStepId % Constants::worldStepsPerUiStep()) == 0);

	RBX::Profiling::Mark mark(*profilingWorldStep, true, true);

	if( !RBX::TaskScheduler::singleton().isCyclicExecutive() )
	{
		worldSteps = std::max(1, Math::iFloor(desiredInterval * Constants::worldStepsPerSec()));
	}

	uiStep(longStep, distributedGameTime);

	if (longStep)
	{
		for (std::set<Assembly*>::iterator i = getMovingAssemblyStage()->getMovingGroundedAssembliesBegin(); i != getMovingAssemblyStage()->getMovingGroundedAssembliesEnd(); i++)
		{
			RBX::Profiling::Mark mark(*contactManager->profilingBroadphase, false);
			RBXASSERT(!((*i)->SimulateStageHook::is_linked()));
			RBXASSERT((*i)->getCurrentStage()->getStageType() != IStage::HUMANOID_STAGE);
			contactManager->onAssemblyMovedFromStep(**i);
		}
	}

	//Time startTime = Time::now<Time::Precise>();
	
	// longStepId is only used for touch signal now
	const int longStepId = getLongUiStepId();

    static const boost::posix_time::ptime epoch = boost::posix_time::min_date_time;
    boost::uint64_t debugTime = ( boost::posix_time::microsec_clock::universal_time() - epoch ).total_milliseconds();

	for (int j = 0; j < worldSteps; j++) 
	{		
		bool throttling = eThrottle.computeThrottle(worldStepId);

		doWorldStep(throttling, longStepId, numThreads, debugTime);

		worldStepId++;
	}

	if (longStep)
	{
		// notify moving for renderer
		notifyMovingAssemblies();
	}
	if(passCount > (double)DFInt::smoothnessReportThreshold)
	{
		sendAnalytics();
	}


#ifdef _WIN32
	RBXASSERT_IF_VALIDATING(_CrtCheckMemory() != 0);
#endif

	return worldSteps * Constants::worldDt();
}


void World::reportTouchInfo(const TouchInfo& info)
{
	touchReporting.append(info);
}

void World::reportTouchInfo(Primitive* p0, Primitive* p1, World::TouchInfo::Type T)
{
	const TouchInfo info = { NULL, NULL, shared_from(PartInstance::fromPrimitive(p0)), shared_from(PartInstance::fromPrimitive(p1)), T };
	touchReporting.append(info);
}

void World::onPrimitiveCollided(Primitive* p0, Primitive* p1)
{
	primitiveCollideSignal(std::make_pair(p0, p1));
}

void AppendFallen(Primitive* p, G3D::Array<Primitive*>* fallen)
{
	fallen->append(p);
};

void World::computeFallen(G3D::Array<Primitive*>& fallen) const
{
	RBXASSERT(fallen.size() == 0);

	const SleepStage* sleepStage = getSleepStage();
	RBXASSERT(sleepStage);

	const SleepStage::AssemblySet& awake = sleepStage->getAwakeAssemblies();

	for (SleepStage::AssemblySet::const_iterator cIt = awake.begin(); cIt != awake.end(); ++cIt) 
	{
		Assembly* assembly = *cIt;
		Vector3 pos = assembly->getAssemblyPrimitive()->getCoordinateFrame().translation;
		if ((pos.y < fallenPartDestroyHeight) || Math::isNanInfVector3(pos))
		{ 
			assembly->visitPrimitives(boost::bind(&AppendFallen, _1, &fallen));
		}
	}
}

void World::insertJoint(Joint* j)
{
	assertNotInStep();

// Re-entrancy problem - please capture and show complete stack to David B.
	RBXASSERT(inJointNotification == NULL);		

	Primitive* p0 = j->getPrimitive(0);
	Primitive* p1 = j->getPrimitive(1);

    // don't destroy additional joints (beyond the first one) if terrain is one of the parts in the joint pair
    if (p0 && p1 && (p0 != p1))
    {
        if( p0->getGeometryType() != Geometry::GEOMETRY_MEGACLUSTER && p1->getGeometryType() != Geometry::GEOMETRY_MEGACLUSTER )
        {
	        if (Joint* alreadyHere = Primitive::getJoint(p0, p1)) 
	        {
		        if (!Joint::isKernelJoint(alreadyHere) && !Joint::isKernelJoint(j) &&
					!Joint::isSpringJoint(alreadyHere) && !Joint::isSpringJoint(j))		// allow multiple kernel joints with other joints
		        {
			        destroyJoint(alreadyHere);		// will fire a notification
		        }
	        }
        }
    }

	Primitive* unGroundedPrimIfGroundedExists = NULL;
	std::vector<Primitive*> combiningRoots;
	bool isKernelJoint = Joint::isKernelJoint(j);
	if (!isKernelJoint)
	{
		gatherMechDataPreJoin(j, unGroundedPrimIfGroundedExists, combiningRoots);
	}

	cleanStage->onEdgeAdded(j);  

	if (!isKernelJoint)
	{
		postInsertJointSignal(j, unGroundedPrimIfGroundedExists, combiningRoots);
	}

	numJoints++;

	if (j->isBreakable()) {
		int ok = breakableJoints.insert(j).second;
		RBXASSERT(ok);
	}

	if (Joint::isKinematicJoint(j))
		j->notifyMoved();
}


// TODO:  performance issue here with lots of glue joints
//
void World::removeFromBreakable(Joint* j)
{
	if (j->isBreakable()) {
		int num = breakableJoints.erase(j);
		RBXASSERT(num == 1);
	}
	else {
		RBXASSERT_SLOW(breakableJoints.find(j) == breakableJoints.end());
	}
}

void World::removeJoint(Joint* j)
{
	assertNotInStep();

	removeFromBreakable(j);

	removeAnimatedJointFromMovingAssemblyStage(j);

	std::vector<Primitive*> prim0Roots;
	std::vector<Primitive*> prim1Roots;
	bool isKernelJoint = Joint::isKernelJoint(j);
	if (!isKernelJoint)
	{
		gatherMechDataPreSplit(j, prim0Roots, prim1Roots);
	}

	cleanStage->onEdgeRemoving(j); 

	if (!isKernelJoint)
	{
		postRemoveJointSignal(j, prim0Roots, prim1Roots);
	}

	numJoints--;
}

void World::gatherMechDataPreJoin(Joint *j, Primitive*& unGroundedPrim, std::vector<Primitive*>& combiningRoots)
{
	Primitive* prim0 = j->getPrimitive(0);
	Primitive* prim1 = j->getPrimitive(1);

	if (prim0 && prim1)
	{
		Primitive* rootPrim0 = prim0->getRootMovingPrimitive();
		Primitive* rootPrim1 = prim1->getRootMovingPrimitive();
		if (rootPrim0 && rootPrim1)
		{
			if (rootPrim0 == rootPrim1)
			{
				combiningRoots.push_back(rootPrim0);
			}
			else
			{
				combiningRoots.push_back(rootPrim0);
				combiningRoots.push_back(rootPrim1);
			}
		}
		else if (Primitive* unGroundedRootPrim = rootPrim0 ? rootPrim0 : rootPrim1)
		{
			unGroundedPrim = rootPrim0 ? prim0 : prim1;
			// If the rootMovingPrimitive is also the mechanismRoot then we know it's safe to clear it's data because ownership
			// transfer is very simple in this case.
			// If it's not a mechanismRoot, then it has to either be descendant of the Joint's primitive OR be the primitive itself
			// Otherwise, the rootMovingPrimitive will still have the same ownership settings after the Join event.
			if ( unGroundedRootPrim == unGroundedPrim->getMechRoot() ||	unGroundedPrim->isAncestorOf(unGroundedRootPrim) ||	unGroundedRootPrim == unGroundedPrim)
			{
				combiningRoots.push_back(unGroundedRootPrim);
			}
		}
	}

}

void World::gatherMechDataPreSplit(Joint* j, std::vector<Primitive*>& prim0ChildRoots, std::vector<Primitive*>& prim1ChildRoots)
{
	// We have to know if either of the Primitives is a parent of the other Primitive.
	// This is so that we can figure out which Ancestor check requires exclusivity when
	// populating the primOwnershipData containers. 
	bool p0ParentOfp1 = false;
	bool p1ParentOfp0 = false;
	Primitive* prim0 = j->getPrimitive(0);
	Primitive* prim1 = j->getPrimitive(1);
	Primitive* rootMovingPrim0 = NULL;
	Primitive* rootMovingPrim1 = NULL;
	if (prim0 && prim1)
	{
		p0ParentOfp1 = prim0->isAncestorOf(prim1);
		p1ParentOfp0 = prim1->isAncestorOf(prim0);
		rootMovingPrim0 = prim0->getRootMovingPrimitive();
		rootMovingPrim1 = prim1->getRootMovingPrimitive();
		if (rootMovingPrim0 && rootMovingPrim1)
		{
			// both rootMovingPrim's existing implies SIMPLE logic for ownership transfer.
			// Lets only use Container 0, since container 1 will have redundant date
			prim0ChildRoots.push_back(rootMovingPrim0);
			return;
		}
	}

	for (int i = 0; i < 2; i++ )
	{
		// Select the correct containers
		std::vector<Primitive*>& primChildRoots						= (i == 0)	? prim0ChildRoots		:	prim1ChildRoots;
		bool parentOfOtherPrim										= (i == 0)	? p0ParentOfp1			:   p1ParentOfp0;
		if (Primitive* prim = (i == 0) ? prim0 : prim1)
		{
			if (Primitive* rootPrim = prim->getMechRoot())
			{
				Assembly* rootAssembly = rootPrim->getAssembly();
				for (int k = 0; k < rootAssembly->numChildren(); k++)
				{
					Assembly* childOfRoot = rootAssembly->getTypedChild<Assembly>(k);
					if (Mechanism::isMovingAssemblyRoot(childOfRoot))
					{
						Primitive* rootOfAssembly = childOfRoot->getAssemblyPrimitive();
						Primitive* otherPrim = (i == 1) ? prim0 : prim1; 
						if (prim->isAncestorOf(rootOfAssembly) && !(parentOfOtherPrim && otherPrim->isAncestorOf(rootOfAssembly)))
						{
							primChildRoots.push_back(rootOfAssembly);
						}
					}
				}
			}
		}
	}
}

void World::notifyMoved(Primitive* p)
{
	if (p == NULL) return;
	p->getOwner()->notifyMoved();

	for (int i = 0; i < p->numChildren(); ++i) {
		Primitive* child = p->getTypedChild<Primitive>(i);
		if (!Assembly::isAssemblyRootPrimitive(child)) {
			notifyMoved(child);
		}
	}
}

void World::jointCoordsChanged(Joint* j)
{
	Primitive* parent = rbx_static_cast<Primitive*>(j->getParentSpanningNode());
	Primitive* child = rbx_static_cast<Primitive*>(j->getChildSpanningNode());

	if (parent && child && (parent != child))
	{
		if (Joint::isRigidJoint(j))
		{						
			RigidJoint* r = rbx_static_cast<RigidJoint*>(j);
			child->getBody()->setMeInParent(r->getChildInParent(parent, child));
			notifyMoved(parent);

				sendClumpChangedMessage(child);
		}
		else
		{
			if (Joint::isMotorJoint(j))
			{					
				RBXASSERT(parent);
				child->getBody()->setMeInParent(j->resetLink());
				notifyMoved(parent);
			}
		}
	}
}

void World::destroyJoint(Joint* j)
{
	RBXASSERT(j);
	inJointNotification = j;
	autoDestroySignal(j);
	inJointNotification = NULL;
}


void World::insertContact(Contact* c)
{
	// RBXASSERT(!inStepCode);
	cleanStage->onEdgeAdded(c);
	numContacts++;
    RBXASSERT( getKernel() != 0 );
    if (getKernel() && getKernel()->getUsingPGSSolver())
    {
        getKernel()->pgsSolver.addContactManifold( c->getPrimitive(0)->getBody()->getUID(), c->getPrimitive(1)->getBody()->getUID() );
    }
}


// Note - should only be called by the contact manager
void World::destroyContact(Contact* c)
{
    if (getKernel() && getKernel()->getUsingPGSSolver())
    {
        getKernel()->pgsSolver.removeContactManifold( c->getPrimitive(0)->getBody()->getUID(), c->getPrimitive(1)->getBody()->getUID() );
    }
	RBXASSERT(!inStepCode);
	cleanStage->onEdgeRemoving(c);
	delete c;
	numContacts--;
}

static bool canSkipJoinAllForPrimitive(Primitive* p)
{
	if (p->getGeometry() && p->getGeometry()->isTerrain())
		return true;
		 
	for (int i = 0; i < NORM_UNDEFINED; ++i)
		if (!IsNoSurface(p->getSurfaceType(static_cast<NormalId>(i))))
			return false;
	
	return true;
}

void World::joinAll() 
{
//	RBXASSERT(numJoints == 0);

	for (int i = 0; i < primitives.size(); ++i) 
	{
		// CSG PHYSICS LOADING HACK
		PartInstance* part = PartInstance::fromPrimitive(primitives[i]);
		if(PartOperation* partOp = RBX::Instance::fastDynamicCast<PartOperation>(part))
		{
				partOp->trySetPhysicsData();
		}
		// CSG PHYSICS LOADING HACK

			if (!canSkipJoinAllForPrimitive(primitives[i])) 
		{
			createAutoJoints(primitives[i]);
		}
	}
	assemble();
}

void World::insertPrimitive(Primitive* p) 
{
	RBXASSERT(!inStepCode);
	RBXASSERT(!p->getAssembly());
	RBXASSERT(!p->inPipeline());
	RBXASSERT(!p->getFirstEdge());
	RBXASSERT(!p->getNumEdges());
	RBXASSERT(!p->getWorld());

    // id's will start at 1, reserving 0 for an invalid value
    UIDGenerator++;
    p->getBody()->setUID( UIDGenerator );
    primitiveIndexation[ UIDGenerator ] = p;

    primitives.fastAppend(p);
	p->setWorld(this);
	p->updateMassValues(getUsingNewPhysicalProperties());

	cleanStage->onPrimitiveAdded(p);

	contactManager->onPrimitiveAdded(p);		// contacts added here

	RBXASSERT(!inStepCode);
}


void World::removePrimitive(Primitive* p, bool isStreamRemove) 
{

	RBXASSERT(!inStepCode);
	RBXASSERT(p->inPipeline());
	RBXASSERT(p->getWorld());

    if( getKernel() && getKernel()->getUsingPGSSolver() )
    {
        getKernel()->pgsSolver.clearBodyCache(p->getBody()->getUID());
    }

	destroyAutoJoints(p, NULL, !isStreamRemove, !isStreamRemove);	

	contactManager->onPrimitiveRemoved(p);		// should destroy all contacts here

	cleanStage->onPrimitiveRemoving(p);

	primitives.fastRemove(p);
    primitiveIndexation.erase( primitiveIndexation.find(p->getBody()->getUID()) );

	p->setWorld(NULL);
    p->getBody()->setUID( 0 );

	RBXASSERT(p->getFirstEdge() == NULL);
	RBXASSERT(p->getNumEdges() == 0);
	RBXASSERT(!inStepCode);
	RBXASSERT(!p->getAssembly());
	RBXASSERT(!p->inPipeline());
	RBXASSERT(!p->getWorld());
}

Primitive* World::getPrimitiveFromBodyUID( boost::uint64_t uid ) const
{
    const auto it = primitiveIndexation.find( uid );
    if( it == primitiveIndexation.end() )
    {
        return NULL;
    }

    return it->second;
}

bool doNotIgnore(Primitive* other, std::set<Primitive*>* ignoreGroup, std::set<Primitive*>* joinGroup)
{
	if (ignoreGroup)
		return ignoreGroup->find(other) == ignoreGroup->end();

	if (joinGroup)
		return joinGroup->find(other) != joinGroup->end();

	return true;
}


void World::destroyAutoJoints(Primitive* p, std::set<Primitive*>* ignoreGroup, bool includeExplicit, bool includeAuto)
{
	RBXASSERT(!inStepCode);
	
	// some scripts might be listening to signals as a result of a joint remove, turning this function recursive
	// we keep tempJoint not as a local variable to avoid having to realloc memory, this list is used like a global stack,
	// each time we enter the function we add more joints to the stack and make sure they are processed and cleared before exiting.

	int startIndex = tempJoints.size();

	for (int i = 0; i < p->getNumJoints(); ++i) {
		Joint* destroyJ = p->getJoint(i);
		RBXASSERT(destroyJ);
		if (destroyJ && ((includeAuto && Joint::isAutoJoint(destroyJ)) || (includeExplicit && Joint::isManualJoint(destroyJ)))) 
		{
			IJointOwner* jointOwner = destroyJ->getJointOwner();
			RBXASSERT(jointOwner);

			if (jointOwner)
			{
				tempJoints.push_back(shared_from(static_cast<JointInstance*>(jointOwner)));
			}
			
		}
	}

	for (int i = startIndex; i < tempJoints.size(); ++i) {
		Joint* destroyJ = tempJoints[i]->getJoint();
		RBXASSERT(destroyJ);
		RBXASSERT(destroyJ->links(p));
		Primitive* other = destroyJ->otherPrimitive(p);

		if (doNotIgnore(other, ignoreGroup, NULL)) {
			destroyJoint(destroyJ);
		}
	}

	if((tempJoints.size() - startIndex) != 0)
		tempJoints.remove(startIndex, tempJoints.size() - startIndex);

	RBXASSERT(startIndex == tempJoints.size());
}

void World::destroyTerrainWeldJointsNoTouch(Primitive* megaClusterPrim, Primitive* touchingPrim)
{
    std::vector<Joint*> destroyTheseJoints;

	int touching = -1;

    for (int j = 0; j < touchingPrim->getNumJoints(); j++)
    {
        Joint* destroyJ = touchingPrim->getJoint(j);
        if (touchingPrim->getJointOther(j) == megaClusterPrim && Joint::isManualJoint(destroyJ))
        {
			if (touching < 0)
				touching = ManualWeldJoint::isTouchingTerrain(megaClusterPrim, touchingPrim);

			if (!touching)
                destroyTheseJoints.push_back(destroyJ);
        }
    }

    for (unsigned int i = 0; i < destroyTheseJoints.size(); i++)
        destroyJoint(destroyTheseJoints[i]);
}

void World::destroyTerrainWeldJointsWithEmptyCells(Primitive* megaClusterPrim, const SpatialRegion::Id& region, Primitive* touchingPrim)
{
    Voxel::Grid* grid = static_cast<MegaClusterInstance*>(megaClusterPrim->getOwner())->getVoxelGrid();

    std::vector<Joint*> destroyTheseJoints;

    for (int j = 0; j < touchingPrim->getNumJoints(); j++)
    {
        Joint* destroyJ = touchingPrim->getJoint(j);
        if (touchingPrim->getJointOther(j) == megaClusterPrim && Joint::isManualJoint(destroyJ))
        {
            Vector3int16 cellPos = static_cast<ManualWeldJoint*>(destroyJ)->getCell();
            
            if (SpatialRegion::regionContainingVoxel(cellPos) == region && grid->getCell(cellPos).solid.getBlock() == Voxel::CELL_BLOCK_Empty)
                destroyTheseJoints.push_back(destroyJ);
        }
    }

    for( unsigned int i = 0; i < destroyTheseJoints.size(); i++ )
        destroyJoint(destroyTheseJoints[i]);
}

// note - leaves contacts intact, but in a non-active state
void World::destroyAutoJoints(Primitive* p, bool includeExplicit)
{
	destroyAutoJoints(p, NULL, includeExplicit);
}

void World::destroyAutoJointsToWorld(const G3D::Array<Primitive*>& primitives)	// ignores joints between them
{
	RBXASSERT(!inStepCode);

	std::set<Primitive*> ignore;
	for (int i = 0; i < primitives.size(); ++i) {
		ignore.insert(primitives[i]);
	}

	for (int i = 0; i < primitives.size(); ++i) {
		destroyAutoJoints(primitives[i], &ignore);
	}
}

void World::createAutoJoints(Primitive* p, std::set<Primitive*>* ignoreGroup, std::set<Primitive*>* joinGroup)
{
	RBXASSERT(!inStepCode);

	numLinkCalls++;
	
	tempPrimitives.fastClear();

	contactManager->getPrimitivesTouchingExtents(p->getFastFuzzyExtents(), p, 0, tempPrimitives);

	// For each other primitive
	for (int i = 0; i < tempPrimitives.size(); ++i) 
	{
		Primitive* other = tempPrimitives[i];

		if (	doNotIgnore(other, ignoreGroup, joinGroup) 
			&&	(!Primitive::getJoint(p, other))	)
		{
			if (Joint* joint = JointBuilder::canJoin(p, other))
			{
				insertJoint(joint);
				inJointNotification = joint;
				autoJoinSignal(joint);
				inJointNotification = NULL;
			}
		}
	}
}

void World::createAutoJoints(Primitive* p)
{
	createAutoJoints(p, NULL, NULL);
}

void World::createAutoJointsToWorld(const G3D::Array<Primitive*>& primitives)	// ignores joints between them
{
	RBXASSERT(!inStepCode);

	std::set<Primitive*> ignore;
	for (int i = 0; i < primitives.size(); ++i) {
		ignore.insert(primitives[i]);
	}

	for (int i = 0; i < primitives.size(); ++i) {
		createAutoJoints(primitives[i], &ignore, NULL);
	}
}

void World::createAutoJointsToPrimitives(const G3D::Array<Primitive*>& primitives)	// only join each other in this group
{
	RBXASSERT(!inStepCode);

	std::set<Primitive*> group;
	for (int i = 0; i < primitives.size(); ++i) {
		group.insert(primitives[i]);
	}

	for (int i = 0; i < primitives.size(); ++i) {
		createAutoJoints(primitives[i], NULL, &group);
	}
}

void World::doBreakJoints() 
{
	std::set<Joint*>::iterator it = breakableJoints.begin();

	while (it != breakableJoints.end())
	{
		Joint* j = *it;
		it++;
		if (j->inKernel() && j->isBroken()) {
			destroyJoint(j);					// removing joint while iterating
		}
	}
}

void World::addAnimatedJointToMovingAssemblyStage(Joint* j)
{
	getMovingAssemblyStage()->addAnimatedJoint(j);
}

void World::removeAnimatedJointFromMovingAssemblyStage(Joint* j)
{
	getMovingAssemblyStage()->removeAnimatedJoint(j);
}

bool World::getUsingPGSSolver() 
{
	RBXASSERT(usingPGSSolver == getKernel()->getUsingPGSSolver());
	return usingPGSSolver;
}

void World::setUsingPGSSolver(bool usePGS) 
{
	usingPGSSolver = usePGS;
	getKernel()->setUsingPGSSolver(usePGS);
}

void World::setUserId( int id )
{
    getKernel()->pgsSolver.setUserId( id );
}

void World::setPhysicalPropertiesMode(PhysicalPropertiesMode mode)
{
	physicalMaterialsMode = mode;
}

bool World::getUsingNewPhysicalProperties() const
{
	if (SFFlag::getNewPhysicalPropertiesForcedOnAll())
		return true;

	PhysicalPropertiesMode legacyMode = SFFlag::getMaterialPropertiesNewIsDefault() ? PhysicalPropertiesMode_Default : PhysicalPropertiesMode_NewPartProperties;
	return (DFFlag::MaterialPropertiesEnabled) ? (getPhysicalPropertiesMode() >= legacyMode) : false;
}

void World::sendAnalytics(void)
{
	if(passCount > 0.0)
	{

		double stopsperk = errorCount * (double)DFInt::smoothnessReportThreshold / passCount;
		RBX::Analytics::EphemeralCounter::reportStats("SmoothnessHumanoidStopsperK", (int)stopsperk, false);
		passCount = 0.0;
		errorCount = 0.0;
	}

	if(frameinfosCount > 0.0)
	{
		RBX::Analytics::EphemeralCounter::reportStats("SmoothnessHumanoidFrameinfosSize", (int)( .5 + frameinfosSize/frameinfosCount), false);
		RBX::Analytics::EphemeralCounter::reportStats("SmoothnessHumanoidFrameinfosTarget", (int)( .5 + frameinfosTarget/frameinfosCount), false);
		RBX::Analytics::EphemeralCounter::reportStats("SmoothnessHumanoidTargetDelayTenths", (int)( .5 + targetDelayTenths/frameinfosCount), false);
		RBX::Analytics::EphemeralCounter::reportStats("SmoothnessHumanoidFrameinfosSizeTenths", (int)( .5 + infosSizeTenths/frameinfosCount), false);
		RBX::Analytics::EphemeralCounter::reportStats("SmoothnessHumanoidMaxDelta", (int)( .5 + maxDelta/frameinfosCount), false);

		frameinfosSize = 0.0;
		frameinfosTarget = 0.0;
		targetDelayTenths = 0.0;
		infosSizeTenths = 0.0;
		maxDelta = 0.0;
		frameinfosCount = 0.0;
	}
}

void World::addFrameinfosStat(double fis, double fit, double tdt, double ist, double md, double fic)
{
	frameinfosSize += fis;
	frameinfosTarget += fit;
	targetDelayTenths += tdt;
	infosSizeTenths += ist;
	maxDelta += md;
	frameinfosCount += fic;

}



} // namespace
