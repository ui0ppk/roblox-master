/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Humanoid/HumanoidState.h"

#include "Humanoid/Humanoid.h"

#include "Humanoid/Running.h"
#include "Humanoid/RunningNoPhysics.h"
#include "Humanoid/GettingUp.h"
#include "Humanoid/FallingDown.h"
#include "Humanoid/Flying.h"
#include "Humanoid/Ragdoll.h"
#include "Humanoid/StrafingNoPhysics.h"
#include "Humanoid/FreeFall.h"
#include "Humanoid/Jumping.h"
#include "Humanoid/Seated.h"
#include "Humanoid/Swimming.h"


#include "V8DataModel/Filters.h"
#include "V8DataModel/Gyro.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8DataModel/GeometryService.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "v8world/Mechanism.h"
#include "V8World/ContactManager.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "V8World/World.h"
#include "V8World/Buoyancy.h"
#include "V8Kernel/Body.h"
#include "V8Kernel/Constants.h"
#include "Network/Players.h"
#include "GfxBase/Adorn.h"	// to do - move to cpp file when it will compile with windows.h in front of it
#include "AppDraw/Draw.h"
#include "AppDraw/DrawAdorn.h"

#include "V8Kernel/Kernel.h"
#include "v8kernel/ContactConnector.h"
#include "v8world/Contact.h"

#include "V8DataModel/HackDefines.h"
#include "Security/FuzzyTokens.h"
#include "Security/JunkCode.h"

#include "VMProtect/VMProtectSDK.h"

DYNAMIC_FASTFLAGVARIABLE(HumanoidFloorPVUpdateSignal, false)
DYNAMIC_FASTFLAGVARIABLE(NoWalkAnimWeld, false)
DYNAMIC_FASTFLAGVARIABLE(ClampRunSignalMinSpeed, false)
DYNAMIC_FASTFLAGVARIABLE(EnableClimbingDirection, false)
DYNAMIC_FASTFLAGVARIABLE(FixJumpGracePeriod, true)
DYNAMIC_FASTFLAGVARIABLE(EnableHipHeight, false)

FASTFLAG(DebugHumanoidRendering)

LOGGROUP(UserInputProfile)
LOGGROUP(HumanoidFloorProcess)
DYNAMIC_FASTFLAG(CheckForHeadHit)
DYNAMIC_FASTFLAG(UseR15Character)

namespace RBX {
	namespace HUMAN {
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Note - padded on top and left for error checking
// Note - also used to determine whether to run "tests" - for example Climbing::Off_Floor signals that climbing needs the floor test
//  Running::Touched signals that running uses this event as well
//

const int STATE_TABLE[ NUM_STATE_TYPES+1 ][ NUM_EVENT_TYPES+1 ] = 
{
xx,					NO_HEALTH,	NO_NECK,	JUMP_CMD,	STRAFE_CMD,			NO_STRAFE_CMD,	SIT_CMD,	NO_SIT_CMD,	PLATFORM_STAND_CMD, NO_PLATFORM_STAND_CMD,	TIPPED,		UPRIGHT,	FACE_LDR,	AWAY_LDR,	OFF_FLOOR,		OFF_FLOOR_GRACE,	ON_FLOOR,	TOUCHED,	NEARLY_TOUCHED,	TOUCHED_HARD,	ACTIVATE_PHYSICS,	FINISHED,	TIMER_UP,		NO_TOUCH_ONE_SECOND, HAS_GYRO,	HAS_BUOYANCY,	NO_BUOYANCY,
																																																																																										
FALLING_DWN,		DEAD,		DEAD,		xx,			xx,					xx,				SEATED,		xx,				PLATFORM_STANDING,	xx,						xx,			xx,			xx,			xx,			xx,			xx,					xx,			xx,			xx,				xx,				xx,					xx,			GETTING_UP,		xx,					xx,			SWIMMING,		xx,
RAGDOLL,			DEAD,		DEAD,		GETTING_UP,	xx,					xx,				xx,			xx,				xx,					xx,						xx,			xx,			xx,			xx,			xx,			xx,					xx,			xx,			xx,				xx,				xx,					xx,			GETTING_UP,		xx,					xx,			xx,				xx,
GETTING_UP,			DEAD,		DEAD,		xx,			xx,					xx,				SEATED,		xx,				PLATFORM_STANDING,	xx,						xx,			RUNNING,	xx,			xx,			xx,			xx,					xx,			xx,			xx,				xx,				xx,					xx,			xx,				xx,					xx,			SWIMMING,		xx,
JUMPING,			DEAD,		DEAD,		xx,			xx,					xx,				SEATED,		xx,				PLATFORM_STANDING,	xx,						xx,			xx,			xx,			xx,			FREE_FALL,	FREE_FALL,			xx,			xx,			xx,				xx,				xx,					FREE_FALL,	FREE_FALL,		xx,					xx,			xx,				xx,
SWIMMING,			DEAD,		DEAD,		JUMPING,	xx,					xx,				SEATED,		xx,				PLATFORM_STANDING,	xx,						xx,			xx,			xx,			xx,			xx,			xx,					xx,			xx,			xx,				RAGDOLL,		xx,					xx,			xx,				xx,					xx,			xx,				GETTING_UP,
FREE_FALL,			DEAD,		DEAD,		xx,			xx,					xx,				SEATED,		xx,				PLATFORM_STANDING,	xx,						xx,			xx,			CLIMBING,	xx,			xx,			xx,					LANDED,		xx,			xx,				xx,				xx,					xx,			xx,				xx,					xx,			SWIMMING,		xx,
FLYING,				DEAD,		DEAD,		xx,			xx,					xx,				SEATED,		xx,				PLATFORM_STANDING,	xx,						FALLING_DWN,xx,			CLIMBING,	xx,			xx,			xx,					RUNNING,	xx,			xx,				xx,				xx,					xx,			xx,				xx,					xx,			SWIMMING,		xx,
LANDED,				DEAD,		DEAD,		xx,			xx,					xx,				SEATED,		xx,				PLATFORM_STANDING,	xx,						FALLING_DWN,xx,			xx,			xx,			xx,			FREE_FALL,			xx,			xx,			xx,				xx,				xx,					xx,			RUNNING,		xx,					xx,			SWIMMING,		xx,
RUNNING,			DEAD,		DEAD,		JUMPING,	xx,					xx,				SEATED,		xx,				PLATFORM_STANDING,	xx,						FALLING_DWN,xx,			CLIMBING,	xx,			xx,			FREE_FALL,			xx,			xx,			xx,				RAGDOLL,		xx,					xx,			xx,				RUNNING_NO_PHYS,	xx,			SWIMMING,		xx,
RUNNING_SLAVE,		DEAD,		DEAD,		JUMPING,	xx,					xx,				SEATED,		xx,				PLATFORM_STANDING,	xx,						FALLING_DWN,xx,			CLIMBING,	xx,			xx,			FREE_FALL,			xx,			xx,			xx,				RAGDOLL,		xx,					xx,			xx,				RUNNING_NO_PHYS,	xx,			SWIMMING,		xx,		// change here for phys
RUNNING_NO_PHYS,	DEAD,		DEAD,		JUMPING,	STRAFING_NO_PHYS,	xx,				SEATED,		xx,				PLATFORM_STANDING,	xx,						xx,			xx,			CLIMBING,	xx,			xx,			FREE_FALL,			xx,			RUNNING,	RUNNING,		xx,				RUNNING,			xx,			xx,				xx,					RUNNING,	SWIMMING,		xx,		// change here for phys
STRAFING_NO_PHYS,	DEAD,		DEAD,		JUMPING,	xx,					RUNNING_NO_PHYS,SEATED,		xx,				PLATFORM_STANDING,	xx,						xx,			xx,			CLIMBING,	xx,			xx,			FREE_FALL,			xx,			RUNNING,	RUNNING,		xx,				RUNNING,			xx,			xx,				xx,					RUNNING,	SWIMMING,		xx,
CLIMBING,			DEAD,		DEAD,		JUMPING,	xx,					xx,				SEATED,		xx,				PLATFORM_STANDING,	xx,						FALLING_DWN,xx,			xx,			RUNNING,	CLIMBING,	CLIMBING,			CLIMBING,	xx,			xx,				xx,				xx,					xx,			xx,				xx,					xx,			SWIMMING,		xx,
SEATED,				DEAD,		DEAD,		JUMPING,	xx,					xx,				xx,			RUNNING,		PLATFORM_STANDING,	xx,						xx,			xx,			xx,			xx,			xx,			xx,					xx,			xx,			xx,				xx,				xx,					xx,			xx,				xx,					xx,			xx,				xx,
PLATFORM_STANDING,	DEAD,		DEAD,		JUMPING,	xx,					xx,				SEATED,		xx,				xx,					RUNNING,				xx,			xx,			xx,			xx,			xx,			xx,					xx,			xx,			xx,				xx,				xx,					xx,			xx,				xx,					xx,			xx,				xx,
DEAD,				xx,			xx,			xx,			xx,					xx,				xx,			xx,				xx,					xx,						xx,			xx,			xx,			xx,			xx,			xx,					xx,			xx,			xx,				xx,				xx,					xx,			xx,				xx,					xx,			xx,				xx,
PHYSICS,			DEAD,		DEAD,		xx,			xx,					xx,				xx,			xx,				xx,					xx,						xx,			xx,			xx,			xx,			xx,			xx,					xx,			xx,			xx,				xx,				xx, 				xx,			xx,				xx,					xx,			xx,				xx	
};

// This function exists as part of a security check.
inline StateType getStateFast(StateType oldState, EventType eventType)
{
	StateType newState = static_cast<StateType>(STATE_TABLE[oldState + 1][eventType + 1]);

	return newState;
}

StateType getState(StateType oldState, EventType eventType)
{
	RBXASSERT(STATE_TABLE[oldState+1][0] == oldState);
	RBXASSERT(STATE_TABLE[0][eventType+1] == eventType);

    return getStateFast(oldState, eventType);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////5////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if CHARACTER_FORCE_DEBUG

void DebugRay::Draw(Adorn* adorn)
{
	if (adorn)
	{
		adorn->setObjectToWorldMatrix(CoordinateFrame(Vector3(0,0,0)));
		adorn->ray(ray, color);
	}
}	

#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////5////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////



HumanoidState::HumanoidState(Humanoid* humanoid, StateType priorState) 
	: humanoid(humanoid)
	, timer(0.0f)
	, noTouchTimer(0.0f)
	, noFloorTimer(0.0f)
	, nearlyTouched(false)
	, finished(false)
	, outOfWater(false)
	, headClear(true)
	, shouldRender(false)				// set to true for debugging
	, lastMovementVelocity(0.0f)
	, priorState(priorState)
	, floorTouchInWorld(unitializedFloorTouch())
	, floorTouchNormal(Vector3::unitY())
	, floorHumanoidLocationInWorld(unitializedFloorTouch())
	, maxForce(0.0f, 0.0f, 0.0f)
	, maxTorque(0.0f, 0.0f, 0.0f)
	, maxContactVel(0.0f)
	, lastForce(0.0f, 0.0f, 0.0f)
	, lastTorque(0.0f, 0.0f, 0.0f)
	, lastContactVel(0.0f)
	, torsoHasBuoyancy( false )
	, leftLegHasBuoyancy( false )
	, rightLegHasBuoyancy( false )
	, ladderCheck(0)
	, luaState(xx)
	, floorMaterial(AIR_MATERIAL)
{
	humanoid->setJump(false);
	humanoid->setStrafe(false);
	setCanThrottleState(false);
	
	PartInstance* torso = getHumanoid()->getVisibleTorsoSlow();
	if (torso)
		buoyancyConnections.push_back( torso->onDemandWrite()->buoyancyChangedSignal.connect( boost::bind(&HumanoidState::setTorsoHasBuoyancy, this, _1 ) ) );

	PartInstance* leftLeg = humanoid->getLeftLegSlow();
	if (leftLeg)
		buoyancyConnections.push_back( leftLeg->onDemandWrite()->buoyancyChangedSignal.connect( boost::bind(&HumanoidState::setLeftLegHasBuoyancy, this, _1 ) ) );

	PartInstance* rightLeg = humanoid->getRightLegSlow();
	if (rightLeg)
		buoyancyConnections.push_back( rightLeg->onDemandWrite()->buoyancyChangedSignal.connect( boost::bind(&HumanoidState::setRightLegHasBuoyancy, this, _1 ) ) );
}

HumanoidState::~HumanoidState()
{
	setCanThrottleState(false);

	std::vector<rbx::signals::connection>::iterator buoyancyConnectionsIterator;
	for( buoyancyConnectionsIterator = buoyancyConnections.begin(); buoyancyConnectionsIterator != buoyancyConnections.end(); ++buoyancyConnectionsIterator )
	{
		(*buoyancyConnectionsIterator).disconnect();
	}
	buoyancyConnections.clear();
}

float HumanoidState::runningKMoveP()
{
	// units: 1/sec        force = kMoveP * mass * velocity
	return 1250.0f;
}

float HumanoidState::runningKMovePForPGS()
{
	return 150.0f;		
}


float HumanoidState::steepSlopeAngle() const
{ 
	if (humanoid)
		return cos(humanoid->getMaxSlopeAngle() * Math::pif() / 180.0f);

	return 0.5f; 
}

void HumanoidState::fireEvents()
{
	if (priorState != getStateType())
	{
		fireEvent(priorState, false);
		fireEvent(getStateType(), true);
		humanoid->stateChangedSignal(priorState, getStateType());
	}

	priorState = getStateType();
}


const Assembly* HumanoidState::getAssemblyConst() const
{
	if (humanoid) {
		if (const Primitive* torsoPrim = humanoid->getTorsoPrimitiveSlow()) {
			return torsoPrim->getConstAssembly();
		}
	}
	return NULL;
}

Assembly* HumanoidState::getAssembly()
{
	if (humanoid) {
		if (Primitive* torsoPrim = humanoid->getTorsoPrimitiveFast()) {
			return torsoPrim->getAssembly();
		}
	}
	return NULL;
}

void HumanoidState::stateToAssembly()
{
	if (Assembly* a = getAssembly()) {
		StateType currentType = getStateType();
		a->setNetworkHumanoidState(static_cast<unsigned char>(currentType));
	}
}


StateType HumanoidState::stateFromAssembly()
{
	if (Assembly* a = getAssembly())
	{
		unsigned char assemblyHumanoidState = a->getNetworkHumanoidState();
		return static_cast<StateType>(assemblyHumanoidState);
	}
	else
	{
		return DEAD;		// Hack - using the assemblies to transmit state information to prevent broadcast
	}
}


// ASSERT the humanoid is always in the world
const Humanoid* HumanoidState::getHumanoidConst() const
{
	RBXASSERT(humanoid);
	return humanoid;
}

void HumanoidState::setCanThrottleState(bool canThrottle)
{
	if (humanoid)
	{
		if (humanoid->getHeadSlow())		{humanoid->getHeadSlow()->getPartPrimitive()->setCanThrottle(canThrottle);}
		if (humanoid->getTorsoSlow())		{humanoid->getTorsoSlow()->getPartPrimitive()->setCanThrottle(canThrottle);}
		if (humanoid->getRightArmSlow())	{humanoid->getRightArmSlow()->getPartPrimitive()->setCanThrottle(canThrottle);}
		if (humanoid->getLeftArmSlow())		{humanoid->getLeftArmSlow()->getPartPrimitive()->setCanThrottle(canThrottle);}
		if (humanoid->getRightLegSlow())	{humanoid->getRightLegSlow()->getPartPrimitive()->setCanThrottle(canThrottle);}
		if (humanoid->getLeftLegSlow())		{humanoid->getLeftLegSlow()->getPartPrimitive()->setCanThrottle(canThrottle);}
	}
}

bool HumanoidState::usesEvent(EventType e) const
{
	StateType myState = getStateType();
	return (getState(myState, e) != xx);
}


void HumanoidState::onComputeForce() {

	Humanoid *humanoid = getHumanoid();
    
    Body* torsoBody = humanoid->getTorsoBodyFast();
    Body* root = humanoid->getRootBodyFast();
	if (root && torsoBody) {
		const CoordinateFrame& rootCoord = root->getCoordinateFrame();
		lastTorque = rootCoord.vectorToObjectSpace(root->getBranchTorque());
		lastForce = root->getBranchForce();
	}

	onComputeForceImpl();
}

void HumanoidState::render3dAdorn(Adorn* adorn)
{
//	if (shouldRender && (humanoid == Humanoid::getLocalHumanoidFromContext(humanoid)))
//	if (shouldRender && Network::Players::backendProcessing(humanoid)) 
	
	if (shouldRender || FFlag::DebugHumanoidRendering) {
		static int frameCount = 0;
		Humanoid *humanoid = getHumanoid();

	    int yBase = 40;
	    if (!humanoid->isLocalSimulating())
		    yBase += 100;

		findLadder(adorn);
		if (floorPart.get()) {
			Draw::selectionBox(floorPart->getPart(), adorn, Color3::white());
			DrawAdorn::cylinder(adorn, CoordinateFrame(floorHumanoidLocationInWorld), 1, 0.2, 1.0, Color3::white());
			Vector3 torsoPos = humanoid->getTorsoBodyFast()->getCoordinateFrame().translation;
			torsoPos.y = getDesiredAltitude();
			DrawAdorn::cylinder(adorn, torsoPos, 1, 0.2, 1.0, Color3::blue());
		}

		if (lastTorque.magnitude() > maxTorque.magnitude() || frameCount == 2047)
			maxTorque = lastTorque;
		if (lastForce.magnitude() > maxForce.magnitude() || frameCount == 2047)
			maxForce = lastForce;
	    if (fabs(lastContactVel) > fabs(maxContactVel) || frameCount == 2047) {
		    maxContactVel = lastContactVel;
		}
		frameCount = (++frameCount) % 1024;

		// debug rays
#if CHARACTER_FORCE_DEBUG
		for(size_t i = 0; i < debugRayList.size(); ++i) {
    		debugRayList[i].Draw(adorn);
		}
		debugRayList.clear();

#endif

		if (humanoid->getHeadSlow()) 
		{
			adorn->drawFont2D(
			    getName().toString(),
			    Vector2(16, yBase),
			    16,		// size
                false,
				Color3::white(),
				Color3::black(),
				Text::FONT_LEGACY,
				Text::XALIGN_LEFT,
				Text::YALIGN_BOTTOM );

		    char buf[256];
		    snprintf(buf, sizeof(buf), "torque(%8.1f, %8.1f, %8.1f), length:%8.1f", maxTorque.x, maxTorque.y, maxTorque.z, maxTorque.length());
		    adorn->drawFont2D(
			    std::string(buf),
			    Vector2(16, yBase + 20),
			    16,		// size
                false,
			    Color3::white(),
			    Color3::black(),
			    Text::FONT_LEGACY,
			    Text::XALIGN_LEFT,
			    Text::YALIGN_BOTTOM );
    
		    snprintf(buf, sizeof(buf), "force(%8.1f, %8.1f, %8.1f), length:%8.1f", maxForce.x, maxForce.y, maxForce.z, maxForce.length());
		    adorn->drawFont2D(
			    std::string(buf),
			    Vector2(16, yBase + 40),
			    16,		// size
                false,
			    Color3::white(),
			    Color3::black(),
			    Text::FONT_LEGACY,
			    Text::XALIGN_LEFT,
			    Text::YALIGN_BOTTOM );

		    snprintf(buf, sizeof(buf), "contact velocity : %f", maxContactVel);
			adorn->drawFont2D(
				std::string(buf),
				Vector2(16, yBase + 60),
				16,		// size
                false,
				Color3::white(),
				Color3::black(),
				Text::FONT_LEGACY,
				Text::XALIGN_LEFT,
				Text::YALIGN_BOTTOM );
		}
	}
}

float HumanoidState::getFloorFrictionProperty(Primitive* floorPrim) const
{
	if (!floorPrim->getPhysicalProperties().getCustomEnabled())
	{
		return MaterialProperties::generatePhysicalMaterialFromPartMaterial(getFloorMaterial()).getFriction();
	}
	return floorPrim->getPhysicalProperties().getFriction();
}

Primitive* HumanoidState::getFloorPrimitive()
{
	if (floorPart.get())
    {
		Primitive* answer = floorPart->getPartPrimitive();
		if (answer->getWorld())
		{
			if (Assembly* humanoidAssembly = getAssembly())
			{
				if (humanoidAssembly != answer->getAssembly())
				{
					return answer;
				}
			}
		}
	}
	return NULL;
}

const Primitive* HumanoidState::getFloorPrimitiveConst() const
{
	if (floorPart.get())
    {
		Primitive* answer = floorPart->getPartPrimitive();
		if (answer->getWorld())
		{
			if (const Assembly* humanoidAssembly = getAssemblyConst())
			{
				if (humanoidAssembly != answer->getAssembly())
				{
					return answer;
				}
			}
		}
	}
	return NULL;
}


HumanoidState* HumanoidState::defaultState(Humanoid* humanoid)
{
	return new Running(humanoid, RUNNING_NO_PHYS);		// will fire an event
}


void HumanoidState::preStepSimulatorSide(float dt)
{
	preStepFloor();

	timer -= dt;
	noTouchTimer += dt;
	noFloorTimer += dt;

	preStepCollide();

	if (DFFlag::CheckForHeadHit)
	{
		headClear = true;
		PartInstance* foundBody = humanoid->getTorsoSlow();
		PartInstance* foundHead = humanoid->getHeadSlow();
		if (foundBody && foundHead) 
		{
			Vector3 headUp = Vector3::unitY() * (foundHead->getPartSizeUi().y / 2.0f);

			FilterHumanoidParts filterHumanoidParts;
			MergedFilter *mergedFilter = new MergedFilter(&filterHumanoidParts, this);
			RbxRay caster(foundHead->getCoordinateFrame().translation, headUp);
			Vector3 hitLocation;
			std::vector<const Primitive*> dummy;
			Primitive* hitPrim = NULL;

			hitPrim = humanoid->getWorld()->getContactManager()->getHit(
				caster, 
				&dummy,
				mergedFilter,
				hitLocation);

			if (mergedFilter)
				delete mergedFilter;

			if (hitPrim != NULL) 
				headClear = false;
		}
	}


	facingLadder = false;
	if (usesLadder()) {
		if (ladderCheck <= 0) {
			facingLadder = findLadder(NULL);
			ladderCheck = ladderCheckRate();
		} 
		else {
			ladderCheck--;
		}
	}
	if (!facingLadder && humanoid->getAutoJump() && enableAutoJump()) 
	{
		Velocity desiredWalkVelocity = 	getHumanoid()->calcDesiredWalkVelocity();
		if (!desiredWalkVelocity.linear.isZero())
			doAutoJump(); 
	}
}

void HumanoidState::preStepCollide()
{
	setLegsCanCollide(legsShouldCollide());	
	setArmsCanCollide(armsShouldCollide());	
	setHeadCanCollide(headShouldCollide());
	setTorsoCanCollide(torsoShouldCollide());
}


void HumanoidState::preStepFloor()
{
	shared_ptr<PartInstance> oldFloor = floorPart;
	floorPart.reset();
	floorTouchInWorld = unitializedFloorTouch();
	floorTouchNormal = Vector3::unitY();
	floorHumanoidLocationInWorld = unitializedFloorTouch();

	if (usesFloor())
		findFloor(oldFloor);

	if (DFFlag::FixJumpGracePeriod && floorPart)
		humanoid->setLastFloorNormal(floorTouchNormal);

	if (oldFloor == floorPart)
		return;

	// changed floor part
	Primitive* floorPrimitive = getFloorPrimitive();

	if (!floorPrimitive)
		return;

	// wake up the part we are on, because we are not doing collisions
	// but instead floating with a force
	humanoid->getWorld()->ticklePrimitive(floorPrimitive, false);
}

// State is master
void HumanoidState::simulate(shared_ptr<HumanoidState>& state, float dt)
{
	RBXASSERT(state.get());

	if (state->getStateType() != state->stateFromAssembly()) {	
		state->stateToAssembly();
	}

	if (state->getAssembly()) {
		state->getAssembly()->wakeUp();			// no longer prevent sleep, instead constantly wake up
	}

	state->preStepSimulatorSide(dt);

	doSimulatorStateTable(state, dt);

	state->onSimulatorStepImpl(dt);						// only for simulating humanoids
	state->onStepImpl();			
	RBXASSERT(!state->getAssembly() || state->getStateType() == state->stateFromAssembly());

	state->humanoid->setActivatePhysics(false, Vector3::zero());
	
	if (DFFlag::HumanoidFloorPVUpdateSignal)
		updateHumanoidFloorStatus(state);
}

bool isValidSubscriptionHumanoidState(shared_ptr<HumanoidState>& state)
{
	// We will be able to include JUMPING once we fix Humanoid Momentum.
	if (state->getStateType() == GETTING_UP ||		state->getStateType() == LANDED ||			state->getStateType() == RUNNING ||
		state->getStateType() == RUNNING_SLAVE ||	state->getStateType() == RUNNING_NO_PHYS ||	state->getStateType() == STRAFING_NO_PHYS ||
		state->getStateType() == PLATFORM_STANDING)
		return true;
	else
		return false;
}


bool checkFloorSubscribable(Primitive* floorPrim, Humanoid* humanoid)
{
	if (floorPrim)
	{
		float humanoidMass = humanoid->getTorsoBodyFast()->getBranchMass();
		float floorMass = floorPrim->getAssembly()->getAssemblyPrimitive()->getBody()->getBranchMass();
		if (PartInstance* floorPart = PartInstance::fromPrimitive(floorPrim))
		{
			if (Humanoid::constHumanoidFromDescendant(floorPart))
			{
				// NO HUMANOID FLOORS PLEASE
				return false;
			}
		}

		FASTLOG2F(FLog::HumanoidFloorProcess, "Check floor subscription Player Mass: %4.4f, Floor Mass: %4.4f", humanoidMass, floorMass);
		if (humanoidMass * 2 < floorMass)
		{
			FASTLOG(FLog::HumanoidFloorProcess, "Floor Subscribed");
			return true;
		}
	}
	return false;
}

void HumanoidState::updateHumanoidFloorStatus(shared_ptr<HumanoidState>& state)
{
	shared_ptr<PartInstance> lastFloor = state->humanoid->getLastFloor();
	Primitive* lastFloorPrim = NULL;
	if (lastFloor)
	{
		lastFloorPrim = lastFloor->getPartPrimitive();
	}

	Primitive* floorPrim = state->getFloorPrimitive();
	bool floorChanged = hasFloorChanged(state, lastFloorPrim);
	// check Disconnect conditions
	if (floorChanged || !isValidSubscriptionHumanoidState(state))
	{
		// Prevents from using Floor Displacement as Velocity
		if (state->humanoid->onPositionUpdatedByNetworkConnection.connected())
		{
			FASTLOG(FLog::HumanoidFloorProcess, "Disconnecting from last floor");
			state->humanoid->onPositionUpdatedByNetworkConnection.disconnect();
		}
		// If we Sit, we still want to remember last floor
		if (state->getStateType() == SEATED)
		{
			//Do nothing
		}
		else if (lastFloor)
		{
			state->humanoid->setLastFloor(NULL);
			state->humanoid->setRootFloorPart(NULL);
			lastFloorPrim = NULL;
		}
	}

	// check Floor update
	if ((floorChanged || lastFloorPrim == NULL) && checkFloorSubscribable(floorPrim, state->humanoid))
	{
		Assembly* floorAssembly = NULL;
		Primitive* floorRootPrim = NULL;
		floorAssembly = floorPrim->getAssembly();
		if (floorAssembly)
		{
			floorRootPrim = floorAssembly->getAssemblyPrimitive();
		}

		if (floorRootPrim)
		{
			state->humanoid->setLastFloor(PartInstance::fromPrimitive(floorRootPrim));
			lastFloorPrim = floorRootPrim;
		}
	}

	// check Subscription conditions
	// We always subscribe to LastFloor because it is guaranteed to be correct, while getFloorPrimitive is not
	if (isValidSubscriptionHumanoidState(state) && lastFloorPrim)
	{
		Assembly* floorAssembly = NULL;
		Primitive* floorRootPrim = NULL;
		floorAssembly = lastFloorPrim->getAssembly();
		if (floorAssembly)
		{
			floorRootPrim = floorAssembly->getAssemblyPrimitive();
			// Need to see RootFloor assembly for checking FilterPhase
			// It's okay for floorAssembly to be NULL
			PartInstance* floorRootPart = PartInstance::fromPrimitive(RBX::Mechanism::getRootMovingPrimitive(lastFloorPrim));
			state->humanoid->setRootFloorPart(floorRootPart);
		}


		if (!state->humanoid->onPositionUpdatedByNetworkConnection.connected())
		{
			state->humanoid->onPositionUpdatedByNetworkConnection = PartInstance::fromPrimitive(floorRootPrim)->onDemandWrite()->onPositionUpdatedByNetworkSignal.connect(boost::bind(&Humanoid::updateNetworkFloorPosition, state->humanoid, _1, _2, _3, _4));
			state->humanoid->setLastFloorPhase(state->humanoid->getCurrentFloorFilterPhase(floorAssembly));
		}
	}
}

bool HumanoidState::hasFloorChanged(shared_ptr<HumanoidState>& state, Primitive* lastFloorPrim)
{
	Primitive* currentFloor = state->getFloorPrimitive();
	// Handles the part if it was deleted.
	if (lastFloorPrim && lastFloorPrim->getWorld() == NULL)
	{
		state->humanoid->setLastFloor(NULL);
		state->humanoid->setRootFloorPart(NULL);
		return true;
	}
	
	if (currentFloor && lastFloorPrim && lastFloorPrim->getAssembly())
	{
		// lastFloorPrim should always be the Assembly root.
		// If there is mismatch, we should disconnect, because the floor HAS changed.
		return ((lastFloorPrim != currentFloor->getAssembly()->getAssemblyPrimitive()) || 
			(lastFloorPrim->getAssembly() != currentFloor->getAssembly()));
	}
	return false;
}


// Assembly is master for state, state is slave
void HumanoidState::noSimulate(shared_ptr<HumanoidState>& state)			// for non-simulating humanoids, match states
{
	RBXASSERT(state.get());

	StateType newType = state->stateFromAssembly();

	if (state->getAssembly()) {
		state->getAssembly()->wakeUp();
	}

	state->preStepSlaveSide();

	doSlaveStateTable(state, newType);

	state->onStepImpl();
}




void HumanoidState::changeState(shared_ptr<HumanoidState>& state, StateType newType)
{
	StateType oldType = state->getStateType();

	if (newType != oldType)
	{
		FASTLOG2(FLog::UserInputProfile, "Changing humanoid state, old: %u, new: %u", oldType, newType);
		Humanoid* humanoid = state->getHumanoid();
		humanoid->setPreviousStateType(oldType);

		state.reset();		// force delete before create - not sure if reset destroys first before creating second?

		state.reset(create(newType, oldType, humanoid));
	}
}


void HumanoidState::doSimulatorStateTable(shared_ptr<HumanoidState>& state, float dt)
{
    VMProtectBeginMutation(NULL);
    bool isBadNvReg = false;
    bool isBadType = false;

	const StateType oldType = state->getStateType();

	if (oldType != state->luaState && state->luaState != xx) {
		changeState(state, state->luaState);
		state->preStepSimulatorSide(dt);		// new state - do it again here
		return;
	}

    Humanoid* humanoid = state->getHumanoid();

    volatile StateType oldTypeSecurityBackup = oldType; // ebx/esi/edi are nonvolatile, but exploiters change them anyways.
	for (int i = 0; i < NUM_EVENT_TYPES; ++i)
	{
		EventType e = static_cast<EventType>(i);
        volatile EventType eBackup = e;

		StateType newType = getState(oldType, e);						// hack - should be in state table
        volatile StateType newTypeBackup = newType;

		RBXASSERT(newType != RUNNING_SLAVE);							// only should come to slave side, never out of state table

        if ((newType != xx) && (newType != oldType) && (humanoid->getStateTransitionEnabled(newType)))
		{
			if (state->computeEvent(e))						// only compute when necessary
			{
				changeState(state, newType);

				state->preStepSimulatorSide(dt);		// new state - do it again here

                isBadType = isBadType || (state->getStateType() != newType); // exploit: wrong object returned

				break;
			}
		}
        isBadNvReg = isBadNvReg || (e != eBackup || newType != newTypeBackup) // exploit: nv reg changed
            || (newType != getStateFast(oldType, e));   // exploit: invalid table jump
	}
    VMProtectEnd();
    VMProtectBeginVirtualization(NULL);
    if (oldType != oldTypeSecurityBackup)
    {
        Tokens::sendStatsToken.addFlagFast(HATE_HSCE_EBX);
        Tokens::simpleToken |= HATE_HSCE_EBX;
    }
    if (isBadType)
    {
        RBX::Security::setHackFlagVmp<LINE_RAND1>(RBX::Security::hackFlag5, HATE_HSCE_EBX);
    }
    if (isBadNvReg)
    {
        Tokens::sendStatsToken.addFlagFast(HATE_HSCE_EBX);
        Tokens::simpleToken |= HATE_HSCE_EBX;
        RBX::Security::setHackFlagVmp<LINE_RAND1>(RBX::Security::hackFlag7, HATE_HSCE_EBX);
    }
    VMProtectEnd();
}

void HumanoidState::doSlaveStateTable(shared_ptr<HumanoidState>& state, StateType newType)
{
	if (state->computeEvent(NO_HEALTH) || state->computeEvent(NO_NECK)) {
		newType = HUMAN::DEAD;
	}
	else if (state->computeEvent(SIT_CMD)) {
		newType = HUMAN::SEATED;
	}
	else if (state->computeEvent(PLATFORM_STAND_CMD)) {
		newType = HUMAN::PLATFORM_STANDING;
	}
	else if (state->getStateType() == RUNNING_NO_PHYS && state->computeTouchedByMySimulation())
		newType = HUMAN::RUNNING_SLAVE;			// ouch - force the server side to go to running mode so we get accurate physics
	else if (state->getStateType() == RUNNING && state->computeEvent(TOUCHED_HARD))
		newType = HUMAN::RAGDOLL;   // Get into ragdoll fast even before the owner says so
	
	if (newType != state->getStateType()) {
		bool keepRunningSlave = ((state->getStateType() == RUNNING_SLAVE) && (newType == RUNNING_NO_PHYS));

		if (!keepRunningSlave) {
			changeState(state, newType);
			state->preStepSlaveSide();
		}
	}
}

bool HumanoidState::computeEvent(EventType eventType) 
{
    // This code is heavily targeted by exploiters, and vmprotect won't obfuscate it.
    // To make it more difficult, junk code will be added randomly each week.
    // VS2012 will write code in file-order.  This means changing/remapping enums will
    // only change the jump table.
    bool returnValue;
	switch (eventType) 
	{
	case HAS_BUOYANCY:			RBX_JUNK; returnValue = computeHasBuoyancy(); break;
	case NO_BUOYANCY:			RBX_JUNK; returnValue = !computeHasBuoyancy(); break;
	case NO_HEALTH:				RBX_JUNK; returnValue = (humanoid->getHealth() <= 0.0f); break;
	case NO_NECK:				RBX_JUNK; returnValue = (!humanoid->getNeck()); break;
	case JUMP_CMD:				RBX_JUNK; returnValue = computeJumped(); break;
	case SIT_CMD:				RBX_JUNK; returnValue = humanoid->getSit(); break;
	case NO_SIT_CMD:			RBX_JUNK; returnValue = !humanoid->getSit(); break;
	case PLATFORM_STAND_CMD:	RBX_JUNK; returnValue = humanoid->getPlatformStanding(); break;
	case NO_PLATFORM_STAND_CMD:	RBX_JUNK; returnValue = !humanoid->getPlatformStanding(); break;
	case TIPPED:				RBX_JUNK; returnValue = computeTipped(); break;
	case UPRIGHT:				RBX_JUNK; returnValue = computeUpright(); break;
	case FACE_LDR:				RBX_JUNK; returnValue = facingLadder; break;
	case AWAY_LDR:				RBX_JUNK; returnValue = !facingLadder; break;
	case STRAFE_CMD:			RBX_JUNK; returnValue = false; break;
	case NO_STRAFE_CMD:			RBX_JUNK; returnValue = !humanoid->getStrafe(); break;
	case ON_FLOOR:				RBX_JUNK; returnValue = (floorPart != NULL) && (!DFFlag::FixJumpGracePeriod || getRelativeMovementVelocity().y <= 0.0f); break;
	case OFF_FLOOR:				RBX_JUNK; returnValue = (floorPart == NULL); break;
	case OFF_FLOOR_GRACE:		RBX_JUNK; returnValue = (noFloorTimer > fallDelay()); break;
	case TOUCHED:				RBX_JUNK; returnValue = computeTouched(); break;
	case NEARLY_TOUCHED:		RBX_JUNK; returnValue = computeNearlyTouched(); break;
	case TOUCHED_HARD:			RBX_JUNK; returnValue = computeTouchedHard(); break;
	case ACTIVATE_PHYSICS:		RBX_JUNK; returnValue = computeActivatePhysics(); break;
	case TIMER_UP:				RBX_JUNK; returnValue = (timer <= 0.0f); break;
	case NO_TOUCH_ONE_SECOND:
		{
            RBX_JUNK;
			computeTouched();			// updates the timer;
			computeNearlyTouched();
			returnValue = (noTouchTimer > 1.0f && !computeHasGyro());
		}
        break;
	case FINISHED:		RBX_JUNK; returnValue = finished; break;
    case HAS_GYRO:		RBX_JUNK; returnValue = computeHasGyro(); break;
	default:
		{
			returnValue = false;
		}
        break;
	}
    RBX_JUNK;
    return returnValue;
}

HumanoidState* HumanoidState::create(StateType newType, StateType oldType, Humanoid* humanoid)
{ 
	HumanoidState* answer = createNew(newType, oldType, humanoid);

	answer->stateToAssembly();

	return answer;
}
	



HumanoidState* HumanoidState::createNew(StateType newType, StateType oldType, Humanoid* humanoid)
{
	RBXASSERT(humanoid);

	switch (newType) 
	{
	case FALLING_DWN:		return new FallingDown(humanoid, oldType);
	case GETTING_UP:		return new GettingUp(humanoid, oldType);
	case JUMPING:			return new Jumping(humanoid, oldType);
	case FREE_FALL:			return new Freefall(humanoid, oldType);
	case FLYING:			return new Flying(humanoid, oldType);
	case RAGDOLL:			return new Ragdoll(humanoid, oldType);
	case LANDED:			return new Landed(humanoid, oldType);
	case RUNNING:			return new Running(humanoid, oldType);
	case RUNNING_SLAVE:		return new RunningSlave(humanoid, oldType);
	case RUNNING_NO_PHYS:	return new RunningNoPhysics(humanoid, oldType);
	case STRAFING_NO_PHYS:	return new StrafingNoPhysics(humanoid, oldType);
	case CLIMBING:			return new Climbing(humanoid, oldType);
	case SEATED:			return new Seated(humanoid, oldType);
	case PLATFORM_STANDING:	return new PlatformStanding(humanoid, oldType);
	case SWIMMING:			return new Swimming(humanoid, oldType);
	case DEAD:				return new Dead(humanoid, oldType);
	case PHYSICS:			return new Physics(humanoid, oldType);
	default:
		{
			RBXASSERT(0);
			return new Running(humanoid, oldType);
		}
	}
}


void HumanoidState::fireEvent(StateType stateType, bool entering)
{
	RBXASSERT(humanoid);

	switch (stateType) 
	{
	case RUNNING:			
	case RUNNING_NO_PHYS:	
		if (DFFlag::ClampRunSignalMinSpeed)
		{
			float moveSpeed = getRelativeMovementVelocity().xz().length();
			float clampedVelocity = (moveSpeed < minMoveVelocity()) ? 0.0f : moveSpeed;

			humanoid->runningSignal(clampedVelocity);
			lastMovementVelocity = clampedVelocity;
		} else {
			humanoid->runningSignal(getRelativeMovementVelocity().xz().length());
		}
		break;
	case CLIMBING:			humanoid->climbingSignal(0.0f);				break;
	case STRAFING_NO_PHYS:	humanoid->strafingSignal(entering);			break;
	case FALLING_DWN:		humanoid->fallingDownSignal(entering);		break;
	case PHYSICS:		
	case RAGDOLL:			humanoid->ragdollSignal(entering);			break;	
	case GETTING_UP:		humanoid->gettingUpSignal(entering);		break;
	case JUMPING:			humanoid->jumpingSignal(entering);			break;
	case FLYING:			
	case LANDED:			
	case FREE_FALL:			humanoid->freeFallingSignal(entering);		break;
	case SEATED:			humanoid->seatedSignal(entering, shared_from(humanoid->getSeatPart()));
		break;
	case PLATFORM_STANDING:	humanoid->platformStandingSignal(entering);	break;
	case DEAD:				humanoid->diedSignal();						break;
	case SWIMMING:			humanoid->swimmingSignal(0.0f);				break;

	default:
		{
			RBXASSERT(0);
		}
	}
}

void HumanoidState::setLuaState(StateType state)
{
	luaState = state;
    void (HumanoidState::* thisFunction)(HUMAN::StateType) = &HumanoidState::setLuaState;
    checkRbxCaller<kCallCheckCodeOnly, callCheckSetApiFlag<kChangeStateApiOffset> >(
        reinterpret_cast<void*>((void*&)(thisFunction)));
}

const char *HumanoidState::getStateNameByType(StateType state) 
{
	switch (state) 
	{
	case RUNNING_NO_PHYS:	return RunningNoPhysics::name().c_str();	break;		
	case RUNNING:			return Running::name().c_str();				break;
	case CLIMBING:			return Climbing::name().c_str();			break;
	case STRAFING_NO_PHYS:	return StrafingNoPhysics::name().c_str();	break;
	case FALLING_DWN:		return FallingDown::name().c_str();			break;
	case RAGDOLL:			return Ragdoll::name().c_str();				break;	
	case GETTING_UP:		return GettingUp::name().c_str();			break;
	case JUMPING:			return Jumping::name().c_str();				break;
	case FLYING:			return Flying::name().c_str();				break;
	case LANDED:			return Landed::name().c_str();				break;
	case FREE_FALL:			return Freefall::name().c_str();			break;
	case SEATED:			return Seated::name().c_str();				break;
	case PLATFORM_STANDING:	return PlatformStanding::name().c_str();	break;
	case DEAD:				return Dead::name().c_str();				break;
	case SWIMMING:			return Swimming::name().c_str();			break;
	case PHYSICS:			return Physics::name().c_str();				break;

	default:
		{
			RBXASSERT(0);
		}
	}
	return NULL;
}

float HumanoidState::computeTilt() const
{
	if (Body* torsoBody = humanoid->getTorsoBodyFast()) 
	{
		Vector3 answer = torsoBody->getCoordinateFrame().vectorToObjectSpace(Vector3::unitY());
		return answer.y;
	}
	else 
	{
		//RBXASSERT(0);
		return 1.0f;
	}
}

float HumanoidState::computeFloorTilt() const
{
	if (getFloorPrimitiveConst()) 
	{
		float tilt = getFloorTouchNormal().dot(Vector3::unitY());
		return tilt;
	}
	else 
	{
		if (DFFlag::FixJumpGracePeriod)
		{
			float tilt = humanoid->getLastFloorNormal().dot(Vector3::unitY());
			return tilt;
		} else {

			return 0.0f;
		}
	}
}

bool HumanoidState::computeTipped() const
{
	float tiltThreshhold = 0.7f;

	if (computeTilt() < tiltThreshhold && !getOutOfWater()) 
	{
		return true;
	}
	else 
	{
		return false;						// old FallCutoff();
	}
}	


bool HumanoidState::computeJumped() const
{
	if ((computeFloorTilt() > steepSlopeAngle() || (getStateType() != RUNNING && getStateType() != RUNNING_NO_PHYS)) && humanoid->getJump() && humanoid->getJumpPower() > 0.0f) 
	{
		return true;
	} 
	else 
	{
		return false;			
	}
}	

bool HumanoidState::computeUpright() const
{
	if (computeTilt() > 0.90f) {			//if (yAxis.y > (1.0 + 2.0 * Balancing::fallCutoff()) / 3.0) 
		return true;
	}
	else {
		return false;
	}
}

bool HumanoidState::computeHasGyro() const
{
	const ModelInstance* character = Humanoid::getConstCharacterFromHumanoid(humanoid);
	RBXASSERT(character);
	const BodyMover* bodyMover = character->findConstFirstDescendantOfType<BodyMover>();
	return (bodyMover != NULL);
}

bool HumanoidState::computeHasBuoyancy()
{
	return ( torsoHasBuoyancy );
}

bool HumanoidState::computeTouchedByMySimulation()
{
	RBXASSERT(humanoid->getWorld());
	if (World* world = humanoid->getWorld()) 
	{
		if (PartInstance* foundTorso = humanoid->getTorsoSlow()) {
			RBX::SystemAddress myLocalAddress = RBX::Network::Players::findLocalSimulatorAddress(humanoid);

			if (world->getContactManager()->intersectingMySimulation(foundTorso->getPartPrimitive(), myLocalAddress, 0.0f)) {
				noTouchTimer = 0.0f;
				return true;
			}

			if (PartInstance* foundHead = humanoid->getHeadSlow()) {
				if (world->getContactManager()->intersectingMySimulation(foundHead->getPartPrimitive(), myLocalAddress, 0.0f)) {
					noTouchTimer = 0.0f;
					return true;
				}
			}
		}
		else {
			RBXASSERT(0);	// no torso - why am I here?
		}
	}
	return false;
}
        
static bool intersectsOutsideOfCharacter(Primitive* check, const Instance* character) {
    Contact* c = check->getFirstContact();
    
    while (c) {
        Primitive* other = c->otherPrimitive(check);
            
        if (	!PartInstance::fromPrimitive(other)->isDescendantOf(character)
            &&	c->computeIsCollidingUi(0.0f)  )
        {
            return true;
        }
        c = check->getNextContact(c);
    }
    return false;
}


bool HumanoidState::computeTouched() 	// only torso, head
{
	RBXASSERT(humanoid->getWorld());

    Instance* character = humanoid->getParent();


	PartInstance* foundTorso = humanoid->getVisibleTorsoSlow();

    if (foundTorso) {
        if (intersectsOutsideOfCharacter(foundTorso->getPartPrimitive(), character)) {
            noTouchTimer = 0.0f;
            return true;
        }

        if (PartInstance* foundHead = humanoid->getHeadSlow()) {
            if (intersectsOutsideOfCharacter(foundHead->getPartPrimitive(), character)) {
                noTouchTimer = 0.0f;
                return true;
            }
        }
    }
    else {
			RBXASSERT(0);	// NO TORSO - why am I here?
    }
	return false;
}

bool HumanoidState::computeNearlyTouched()
{
	if (humanoid->computeNearlyTouched())
	{
		noTouchTimer = 0.0f;
		return true;
	}
	return false;
}

bool HumanoidState::computeTouchedHard()
{ 
	return humanoid->getTouchedHard();
}

bool HumanoidState::computeActivatePhysics()
{
	return humanoid->getActivatePhysics();
}

// Ragdoll Entrance Criteria - make it conservative
//
bool HumanoidState::computeHitByHighImpactObject()
{
	Body* root = getHumanoid()->getRootBodyFast();
	Velocity rootVelocity = root->getVelocity();

	PartInstance* part = humanoid->getTorsoSlow();
	Primitive* humanPrim = part ? part->getPartPrimitive() : NULL;
	if ( humanPrim ) 
	{
		for (int i = 0; i < humanPrim->getNumContacts(); ++i )
		{
			Contact* contact = humanPrim->getContact(i);
			RBXASSERT( contact );

			for( int j = 0; j < contact->numConnectors(); ++j )
			{
				ContactConnector* connector = contact->getConnector(j);
				RBXASSERT( connector );

				lastContactVel = connector->computeRelativeVelocity();
				if ( lastContactVel <= getHumanoid()->getRagdollCriteria() )
					continue;	// Slow contact speed, reject

				Vector3 position, normal;
				float length;
				connector->getLengthNormalPosition(position, normal, length);

				Vector3 humanVelocity = humanPrim->getConstBody()->getPvFast().linearVelocityAtPoint(position);
				float humanNormalVel = normal.dot(humanVelocity);
				if ( humanNormalVel < 0.0f )
					humanNormalVel = -humanNormalVel;

				const Primitive* objectPrim = contact->getConstPrimitive(0) == humanPrim ? contact->getConstPrimitive(1) : contact->getConstPrimitive(0);				
				Vector3 objectVelocity = objectPrim->getConstBody()->getPvFast().linearVelocityAtPoint(position);
				float objectNormalVel = normal.dot(objectVelocity);
				if ( objectNormalVel < 0.0f )
					objectNormalVel = -objectNormalVel;

				if ( humanNormalVel >= objectNormalVel )
					continue;	// Fast human and slow object, reject
				return true; // Slow human hit by fast object
			}
		}
	}

	return false;
}


void HumanoidState::findFloor(shared_ptr<PartInstance>& oldFloor)
{
	if (Primitive* torsoPrim = humanoid->getTorsoPrimitiveSlow())
	{
		const CoordinateFrame& torsoC = torsoPrim->getCoordinateFrame();

		Vector3 halfSize = 0.4f * torsoPrim->getSize();

		float maxDistance = 1.0;

		// checking if I had a floor last check
		float hysteresis = (oldFloor != NULL) ? 1.5f : 1.1f;

		// Increase the hysteresis based on our vertical velocity
		float verticalVelocity = fabs(torsoPrim->getBody()->getVelocity().linear.y);
		if (verticalVelocity > 100.0f)
			hysteresis += verticalVelocity / 100.0f;

		if (PartInstance* foundLeftLeg = humanoid->getLeftLegSlow()) {
			maxDistance += hysteresis * foundLeftLeg->getPartPrimitive()->getSize().y;
		}
		else if (PartInstance* foundRightLeg = humanoid->getRightLegSlow()) {
			maxDistance += hysteresis * foundRightLeg->getPartPrimitive()->getSize().y;
		}

		if (DFFlag::EnableHipHeight)
		{
			maxDistance += (hysteresis * humanoid->getHipHeight());
		}

		Assembly* humanoidAssembly = torsoPrim->getAssembly();

		Vector3 floorHitLocationAccumulate;
		int floorHitCount = 0;

		// check in direction of motion
		float zOffset[] = { 0.0f, 1.0f, -1.0f, 2.0f, -2.0f };  // order the rays to be closest to center first
		for (int z = 0; z <= 4; z++) 
		{
			Vector3 offset (0.0f, -halfSize.y, zOffset[z] * halfSize.z);
			bool UpdateFloorPart = (zOffset[z] < 2.0f && zOffset[z] > -2.0f);
			AverageFloorRayCast(floorPart, floorTouchInWorld, floorTouchNormal, floorMaterial, floorHitLocationAccumulate, floorHitCount, UpdateFloorPart, 
								offset, maxDistance, humanoidAssembly, torsoC);
		}
			
		if (floorPart == NULL)
		{
			// if nothing on the center line, check for shoulders
			for (int x = -1; x <= 1; x+=2) {
				for (int z = -1; z <= 1; z+=2) 
				{
					Vector3 offset (x * halfSize.x, -halfSize.y, z * halfSize.z);
					AverageFloorRayCast(floorPart, floorTouchInWorld, floorTouchNormal, floorMaterial, floorHitLocationAccumulate, floorHitCount, true, offset, 
											maxDistance, humanoidAssembly, torsoC);
				}
			}
		}

		if (floorPart != NULL)
		{
			floorHumanoidLocationInWorld = floorHitLocationAccumulate / (float) floorHitCount;
			noFloorTimer = 0.0f;
			return;
		}

	}
}

// Filters body parts during hit tests with floor
HitTestFilter::Result HumanoidState::filterResult(const Primitive* testMe) const
{
	RBXASSERT(testMe);

	HitTestFilter::Result newResult = HitTestFilter::INCLUDE_PRIM;

	if (	(!testMe->getCanCollide())
		||	(filteringAssembly && (filteringAssembly == testMe->getConstAssembly())))
	{
		newResult = HitTestFilter::IGNORE_PRIM;
	}

	return newResult;
}

void HumanoidState::AverageFloorRayCast(shared_ptr<PartInstance> &floorPart, Vector3& floorPartHitLocation, Vector3& floorPartHitNormal, 
										PartMaterial& floorPartHitMaterial, Vector3& hitLocationAccumulator, int& hitLocationCount, const bool UpdateFloorPart, 
										const Vector3& offset, const float maxDistance,	Assembly* humanoidAssembly, const CoordinateFrame& torsoC)
{
	shared_ptr<PartInstance> floor;
	PartMaterial floorHitMaterial;
	Vector3 floorHitLocation;
	Vector3 floorHitNormal;
	Vector3 pointOnTorso = torsoC.pointToWorldSpace(offset);				

	RbxRay tryRay = RbxRay::fromOriginAndDirection(pointOnTorso, -Vector3::unitY());
	if ((floor = tryFloor(tryRay, floorHitLocation, floorHitNormal, maxDistance, humanoidAssembly, floorHitMaterial)))
	{
		hitLocationAccumulator += floorHitLocation;
		hitLocationCount++;
		if (UpdateFloorPart && floorPart == NULL)
		{
			floorPartHitMaterial = floorHitMaterial;
			floorPartHitLocation = floorHitLocation;
			floorPartHitNormal = floorHitNormal;
			floorPart = floor;
		}
	}
}


shared_ptr<PartInstance> HumanoidState::tryFloor(const RbxRay& ray, Vector3& hitLocation, Vector3& hitNormal, float maxDistance, Assembly* humanoidAssembly, PartMaterial& recentFloorMaterial)
{
	RBXASSERT(humanoid->getWorld());

	RBXASSERT(humanoidAssembly);
	filteringAssembly = humanoidAssembly;
	Primitive* floorPrim = NULL;

	RBXASSERT(ray.direction().isUnit());
	RbxRay worldRay = RbxRay::fromOriginAndDirection(ray.origin(), ray.direction());
	worldRay.direction() *= maxDistance;

	floorPrim =  humanoid->getWorld()->getContactManager()->getHit(worldRay,
							NULL,
							this,
							hitLocation,
							false,
							true,
							hitNormal,
							recentFloorMaterial);

	shared_ptr<PartInstance> answer;
	if (floorPrim && (floorPrim->getAssembly() != humanoidAssembly)) {
		answer = shared_from<PartInstance>(PartInstance::fromPrimitive(floorPrim));
	}
	return answer;
}

// This works even when throttling and the velocity is not in synch with the character velocity
const Velocity HumanoidState::getFloorPointVelocity() 
{
	RBXASSERT(humanoid->getWorld());

	if (Primitive* p = getFloorPrimitive()) 
	{
		RBXASSERT(p->getWorld());
		if (p->getWorld())
		{
			RBXASSERT(floorTouchInWorld != unitializedFloorTouch());

			World* world = humanoid->getWorld();

			Velocity throttledV;
			if (DFFlag::HumanoidFloorPVUpdateSignal && getHumanoid()->shouldNotApplyFloorVelocity(p))
			{
				throttledV = Velocity(Vector3(0,0,0), Vector3(0,0,0));
			}
			else
			{
				throttledV = p->getPV().velocityAtPoint(floorTouchInWorld);		// this velocity could be inaccurate
				float throttle = world->getEnvironmentSpeed();

				throttledV.linear *= throttle;
				throttledV.rotational *= throttle;
			}

			return throttledV;
		}
	}
	return Velocity::zero();
}

Vector3 HumanoidState::getRelativeMovementVelocity()
{
	RBX::Primitive *torso = humanoid->getTorsoPrimitiveSlow();
	if (torso)
	{
		if (DFFlag::NoWalkAnimWeld && torso->getAssembly() && torso->getAssembly()->getAssemblyState() == Sim::ANCHORED)
			return Vector3::zero();

		Vector3 walkVelocity = torso->getBody()->getVelocity().linear;
		Vector3 floorVelocity;
		floorVelocity = getFloorPointVelocity().linear;
		return (walkVelocity - floorVelocity);
	}
	else
	{
		return Vector3::zero();
	}
}

void HumanoidState::fireMovementSignal(rbx::signal<void(float)>& movementSignal, float movementVelocity)
{
	if (humanoid->getTorsoPrimitiveSlow() &&
		!movementSignal.empty()) 
	{
		if (DFFlag::EnableClimbingDirection)
		{
			float clampedVelocity = (std::abs(movementVelocity) < minMoveVelocity()) ? 0.0f : movementVelocity;
			float absVelocity = std::abs(clampedVelocity);

			//This fixes an issue where changing climbing direction instantly will make your direction not ever hit 0, causing the absolute checks to not see a change.
			bool opposites = ((lastMovementVelocity < 0 && clampedVelocity > 0) || (lastMovementVelocity > 0 && clampedVelocity < 0));
			float absMovementVelocity = std::abs(lastMovementVelocity);
			float low = absMovementVelocity * 0.95f;
			float high = absMovementVelocity * 1.05f;
		

			if ((absVelocity < low) || (absVelocity > high) || opposites)
			{
				movementSignal(clampedVelocity);
   				lastMovementVelocity = clampedVelocity;
			}
		}
		else
		{
			float clampedVelocity = (movementVelocity < minMoveVelocity()) ? 0.0f : movementVelocity;

			float low = lastMovementVelocity * 0.95f;
			float high = lastMovementVelocity * 1.05f;
			if ((clampedVelocity < low) || (clampedVelocity > high))
			{
				movementSignal(clampedVelocity);
				lastMovementVelocity = clampedVelocity;
			}
		}
	}
}

float HumanoidState::getCharacterHipHeight() const
{
	float height = 0.0f;

	if (Primitive* p = humanoid->getTorsoPrimitiveSlow())
		height += 0.5f * p->getSize().y;

	if (Primitive* p = PartInstance::getPrimitive(humanoid->getLeftLegSlow())) {
		height += p->getSize().y;
	}
	else if ((p = PartInstance::getPrimitive(humanoid->getRightLegSlow()))) {
		height += p->getSize().y;
	}

	if (DFFlag::EnableHipHeight)
	{
		height += humanoid->getHipHeight();
	}

	return height;
}

float HumanoidState::getDesiredAltitude() const
{
	float desiredAltitude = getFloorHumanoidLocationInWorld().y;

	return desiredAltitude + getCharacterHipHeight();
}

// Some constants
// Nyquist rate = 6 samples per stud, use slightly more so we're not integer stud aligned
float sampleSpacing() {return 1.0f / 7.0f;}
float lowLadderSearch(HumanoidState *pHumanoidState)	
{
	if (DFFlag::EnableHipHeight)
	{
		return -0.3f + pHumanoidState->getCharacterHipHeight();	
	} else {
		return 2.7;			// tweaked - not, should take leg length into account!
	}
}

float searchDepth(Humanoid *pHumanoid) {
    if (pHumanoid && pHumanoid->getUseR15())
	{
		return 1.2f;
	} 
	else 
	{
		return 0.7f;
	}
}								// studs from the middle of leg to the max depth to search for a rung or step
float ladderSearchDistance(Humanoid *pHumanoid) {return searchDepth(pHumanoid) * 1.5f; }	//  1.5x search depth


bool HumanoidState::findPrimitiveInLadderZone(Adorn* adorn)
{
    
	Body* torsoBody = humanoid->getTorsoPrimitiveSlow()->getBody();
	RBXASSERT(torsoBody);

	const CoordinateFrame& cf = torsoBody->getCoordinateFrame();
	GeometryService *geom = ServiceProvider::find<GeometryService>(humanoid);
	RBXASSERT(geom);

	float bottom = -lowLadderSearch(this);
	float top = 0.0f;
	float radius = 0.5f * ladderSearchDistance(humanoid);
	Vector3 center = cf.translation + (cf.lookVector() * ladderSearchDistance(humanoid) * 0.5f);
	Vector3 min(-radius, bottom, -radius);
	Vector3 max(radius, top, radius);
	
	Extents extents(center + min, center + max);

	foundParts.fastClear();
	geom->getPartsTouchingExtents(extents, NULL, 0, foundParts);
	
	Instance* character = getHumanoid()->getParent();
	for (int i = 0; i < foundParts.size(); ++i) {
		PartInstance* foundPart = foundParts[i];
		if (!foundPart->isDescendantOf(character)) {
			if (adorn) {
				Draw::selectionBox(foundPart->getPart(), adorn, Color3::red());
			}
			return true;
		}
	}

	// only show if nothing touching, otherwise go to ray search
	if (adorn) {
		adorn->setObjectToWorldMatrix(CoordinateFrame());
		adorn->box(
			AABox(extents.min(), extents.max())
			);
	}
	return false;

}

void HumanoidState::doLadderRaycast(GeometryService *geom, const RbxRay& caster,
		Humanoid* humanoid, Primitive** hitPrimOut, Vector3* hitLocationOut) 
{
	if (humanoid->getWorld()) {
		FilterDescendents filterDescendents(shared_from(humanoid->getParent()));
		FilterHumanoidParts filterHumanoidParts;

		MergedFilter *mergedFilter = new MergedFilter(&filterHumanoidParts, this);

		std::vector<const Primitive*> dummy;

		*hitPrimOut = humanoid->getWorld()->getContactManager()->getHit(
			caster, 
			&dummy,
			mergedFilter,
			*hitLocationOut);

		if (mergedFilter)
			delete mergedFilter;
	} else {
		*hitLocationOut = Vector3::zero();
	}
}

void HumanoidState::doAutoJump() 
{
	if(!humanoid) {
		return;
	}

	if (!humanoid->getAutoJumpEnabled() )
		return;

	float rayLength = 1.5; 
	// This is how high a ROBLOXian jumps from the mid point of his torso 
	float jumpHeight = 7.0; 

	PartInstance* torso = humanoid->getTorsoSlow(); 
	if(!torso) {
		return; 
	}

	const CoordinateFrame& torsoCFrame = torso->getCoordinateFrame();
	Vector3 torsoLookVector = torsoCFrame.lookVector(); 
	Vector3 torsoPos = torsoCFrame.translation; 

	GeometryService *geom = ServiceProvider::create<GeometryService>(humanoid); 
	if (geom) 
	{
		shared_ptr<PartInstance> hitPart; 
		shared_ptr<PartInstance> jumpHitPart; 

		RbxRay torsoRay(torsoPos + Vector3(0, -torso->getConstPartPrimitive()->getSize().y/2, 0), torsoLookVector * rayLength); 
		RbxRay jumpRay(torsoPos + Vector3(0, jumpHeight - torso->getConstPartPrimitive()->getSize().y, 0), torsoLookVector * rayLength); 
		
        Vector3 unusedSurfaceNormal;
        PartMaterial unusedSurfaceMaterial;
        
		geom->getHitLocationPartFilterDescendents(torso->getParent(), torsoRay, hitPart, unusedSurfaceNormal, unusedSurfaceMaterial, false, false);
		geom->getHitLocationPartFilterDescendents(torso->getParent(), jumpRay, jumpHitPart, unusedSurfaceNormal, unusedSurfaceMaterial, false, false);

		if(hitPart && !jumpHitPart && hitPart->getCanCollide()) 
		{			
			humanoid->setJump(true); 			
		}
	}		
}

bool HumanoidState::findLadder(Adorn* adorn)
{
    if (!humanoid->getStateTransitionEnabled(HUMAN::CLIMBING))
        return false;

	PartInstance* foundBody = humanoid->getTorsoSlow();
	if (!foundBody) 
		return false;

	// First Check - blow out if no primitives other than floor found
	if (!findPrimitiveInLadderZone(adorn))
		return false;

	const CoordinateFrame& torsoCoord = foundBody->getCoordinateFrame();
	Vector3 torsoLook = torsoCoord.lookVector();

	GeometryService *geom = ServiceProvider::create<GeometryService>(humanoid);
	
	// climbable conditions
	float first_space = 0.0f; // needs to be < 2.45f
	float first_step = 0.0f; // needs to be > 0 and < 2.45f
	float first_step_mag = -1.0f;
	float second_step = -1.0f;

	bool look_for_space = true;
	bool look_for_step = false;

	Color3 color = Color3::orange();	// only when rendering
	float heightScale = getCharacterHipHeight() / 3.0f;
	float spacing = sampleSpacing() * heightScale;
	
	int topRay = static_cast<int>(lowLadderSearch(this) / spacing);
	topRay += 10;

	for (int i = 1; i < topRay; i++) // search for 3 studs, starting at toes		NOTE - removed bottom search
	{
		float distanceFromBottom = (float)i * spacing;
		Vector3 originOnTorso(0, -lowLadderSearch(this) + distanceFromBottom, 0);
		RbxRay caster(torsoCoord.translation + originOnTorso, torsoLook * ladderSearchDistance(humanoid) );

		Primitive* hitPrim = NULL;
		Vector3 hitLoc;
		doLadderRaycast(geom, caster, humanoid, &hitPrim, &hitLoc);

		// make trusses climbable.
		if (hitPrim) 
		{
			PartInstance *p = PartInstance::fromPrimitive(hitPrim);
			if (p)
			{
				if (p->getPartType() == RBX::TRUSS_PART) return true;
			}
		}

		float mag = (hitLoc - caster.origin()).magnitude();

		if (hitPrim && mag < searchDepth(humanoid)) {
			// this is a step or rung
			color = Color3::gray();
			if (look_for_space) 
			{
				// we were looking for a space and now we've found the end of it
				first_space = distanceFromBottom;
				look_for_space = false;
				look_for_step = true;
				color = Color3::green();
				first_step_mag = mag;
			} 
			else if (!look_for_step && second_step < 0.0f) 
			{
				second_step = distanceFromBottom;
				color = Color3::blue();
			}
		} 
		else
		{
			color = Color3::yellow();

			// this is a space
			if (look_for_step)
			{
				// we were looking for a step and now we've found the end of it
				first_step = distanceFromBottom - first_space;
				look_for_step = false;
				color = Color3::red();
			}
		}

		// debug ray to show probes
		if (adorn) { 
			adorn->setObjectToWorldMatrix(CoordinateFrame(Vector3(0,0,0)));\

			if (mag < searchDepth(humanoid))
			{
				caster.direction() = torsoLook * mag;
			}
			adorn->ray(caster, color);
		}

		/*
		if (adorn) { 
			adorn->setObjectToWorldMatrix(torsoCoord.translation);
			RbxRay rayInPart = RbxRay::fromOriginAndDirection(	caster.origin() - torsoCoord.translation, 
															hitLoc - caster.origin()	);
			adorn->ray(rayInPart, color);
		}
		*/
	}

	if (first_space < maxClimbDistance() * heightScale &&
		first_step > 0.0f &&
		first_step < maxClimbDistance() * heightScale &&
		(second_step >= 0.0f || first_space > (maxClimbDistance() * heightScale / 5.0f))) // did we find a second step or was the first step up high?
	{
		if (adorn) {
			DrawAdorn::cylinder(adorn, CoordinateFrame(torsoCoord.translation + Vector3(0,5,0)), 1, 0.2, 1.0, Color3::purple());
		}
		return true;
	}

	return false;
}

void HumanoidState::setLegsCanCollide(bool canCollide)
{
	// TODO: put somewhere else, only when leg added/removed
	if (PartInstance* leftLeg = getHumanoid()->getLeftLegSlow())
		leftLeg->getPartPrimitive()->setPreventCollide(!canCollide);

	if (PartInstance* rightLeg = getHumanoid()->getRightLegSlow())
		rightLeg->getPartPrimitive()->setPreventCollide(!canCollide);
}

void HumanoidState::setArmsCanCollide(bool canCollide)
{
	// TODO: put somewhere else, only when leg added/removed
	if (PartInstance* leftArm = getHumanoid()->getLeftArmSlow())
		leftArm->getPartPrimitive()->setPreventCollide(!canCollide);

	if (PartInstance* rightArm = getHumanoid()->getRightArmSlow())
		rightArm->getPartPrimitive()->setPreventCollide(!canCollide);
}

void HumanoidState::setHeadCanCollide(bool canCollide)
{
	if (PartInstance* head = getHumanoid()->getHeadSlow())
		head->getPartPrimitive()->setPreventCollide(!canCollide);
}

void HumanoidState::setTorsoCanCollide(bool canCollide)
{
	if (DFFlag::UseR15Character && getHumanoid()->getUseR15())
	{
		PartInstance* lowertorso = Instance::fastDynamicCast<PartInstance>(getHumanoid()->getParent()->findFirstChildByName("LowerTorso"));
		PartInstance* uppertorso = getHumanoid()->getVisibleTorsoSlow();
		if (lowertorso)
			lowertorso->getPartPrimitive()->setPreventCollide(!canCollide);
		if (uppertorso)
			uppertorso->getPartPrimitive()->setPreventCollide(!canCollide);
	} else {
		PartInstance* torso = getHumanoid()->getVisibleTorsoSlow();
		if (torso)
			torso->getPartPrimitive()->setPreventCollide(!canCollide);
	}
}

void HumanoidState::setNearlyTouched()
{
	nearlyTouched = true;
}

#pragma optimize("s", on)
// prevent inlining of computeEvent
unsigned int HumanoidState::checkComputeEvent()
{
    VMProtectBeginMutation(NULL);
    // these all return true:
    torsoHasBuoyancy = true;
    facingLadder = true;
    humanoid->sit = true;
    humanoid->platformStanding = true;
    bool positiveTests = computeEvent(HAS_BUOYANCY) && !computeEvent(NO_BUOYANCY) 
        && computeEvent(OFF_FLOOR) && computeEvent(SIT_CMD) 
        && computeEvent(PLATFORM_STAND_CMD) && computeEvent(FACE_LDR);

    // these all return false
    torsoHasBuoyancy = false;
    facingLadder = false;
    humanoid->sit = false;
    humanoid->platformStanding = false;
    bool negativeTests = !(computeEvent(HAS_BUOYANCY) || !computeEvent(NO_BUOYANCY) 
        || computeEvent(ON_FLOOR) || computeEvent(SIT_CMD) 
        || computeEvent(PLATFORM_STAND_CMD) || computeEvent(FACE_LDR));

    volatile unsigned int returnValue = static_cast<unsigned int>(positiveTests) + static_cast<unsigned int>(negativeTests);
    VMProtectEnd();
    return returnValue;
}
#pragma optimize("", on)


} // namespace HUMAN
} // namespace RBX
