/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Humanoid/Humanoid.h"
#include "Humanoid/HumanoidState.h"
#include "Humanoid/Jumping.h"

#include "V8DataModel/GameBasicSettings.h"
#include "V8DataModel/BasicPartInstance.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/ForceField.h"
#include "V8DataModel/Backpack.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/Filters.h"
#include "V8DataModel/Teams.h"
#include "V8DataModel/DataModelMesh.h"
#include "V8DataModel/Decal.h" // TODOFACE
#include "V8DataModel/Value.h"
#include "V8DataModel/Animator.h"
#include "v8datamodel/Attachment.h"
#include "V8World/Contact.h"
#include "V8World/World.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "V8World/Joint.h"
#include "AppDraw/DrawAdorn.h"
#include "GfxBase/Adorn.h"
#include "GfxBase/AdornSurface.h"
#include "Util/Rect.h"
#include "Util/Color.h"
#include "Util/Region2.h"
#include "Util/SoundChannel.h"
#include "Util/Analytics.h"
#include "V8Kernel/Constants.h"
#include "V8Kernel/Body.h"
#include "Gui/ProfanityFilter.h"
#include "rbx/make_shared.h"
#include "V8DataModel/UserInputService.h"
#include "V8Kernel/Kernel.h"

#include "Network/Players.h"

#include "rbx/Profiler.h"

#include "Util/PathInterpolatedCFrame.h"

LOGGROUP(HumanoidFloorProcess)
LOGGROUP(UserInputProfile)

LOGGROUP(DistanceMetricP1)

DYNAMIC_FASTINTVARIABLE(HumanoidFloorTeleportWeightValue, 50);
DYNAMIC_FASTINTVARIABLE(HumanoidFloorManualFrictionVelocityMultValue, 100);
FASTFLAGVARIABLE(PhysicsSkipNonRealTimeHumanoidForceCalc, false)

DYNAMIC_FASTFLAG(UseR15Character)

DYNAMIC_FASTFLAGVARIABLE(HumanoidCookieRecursive, false)
DYNAMIC_FASTFLAGVARIABLE(ReplicateLuaMoveDirection, false)
DYNAMIC_FASTFLAGVARIABLE(NamesOccludedAsDefault, false)
DYNAMIC_FASTFLAGVARIABLE(FixedSitFirstPersonMove, true)
DYNAMIC_FASTFLAGVARIABLE(HumanoidCheckForNegatives, true)

DYNAMIC_FASTFLAGVARIABLE(EnableMotionAnalytics, false)
DYNAMIC_FASTFLAGVARIABLE(Enable2edHumanoidDistanceLogging, false)
DYNAMIC_FASTINTVARIABLE(MotionDiscontinuityThreshold, 1)	/* in tenths of a second */

DYNAMIC_FASTFLAGVARIABLE(RotateFirstPersonInVR, true)
FASTFLAGVARIABLE(HumanoidRenderBillboard, false)
FASTFLAGVARIABLE(HumanoidRenderBillboardVR, true)

namespace RBX {

	// TODO: Create template version of normalIdToMatrix3 to avoid the switch statement
static CoordinateFrame rightShoulderP(	normalIdToMatrix3(NORM_X), Vector3(1, 0.5, 0));
static CoordinateFrame leftShoulderP(	normalIdToMatrix3(NORM_X_NEG), Vector3(-1, 0.5, 0));
static CoordinateFrame rightHipP(		normalIdToMatrix3(NORM_X), Vector3(1, -1, 0));
static CoordinateFrame leftHipP(		normalIdToMatrix3(NORM_X_NEG), Vector3(-1, -1, 0));
static CoordinateFrame neckP(			normalIdToMatrix3(NORM_Y), Vector3(0, 1, 0));

static CoordinateFrame rightArmP(		normalIdToMatrix3(NORM_X), Vector3(-0.5, 0.5, 0));
static CoordinateFrame leftArmP(		normalIdToMatrix3(NORM_X_NEG), Vector3(0.5, 0.5, 0));
static CoordinateFrame rightLegP(		normalIdToMatrix3(NORM_X), Vector3(0.5, 1, 0));
static CoordinateFrame leftLegP(		normalIdToMatrix3(NORM_X_NEG), Vector3(-0.5, 1, 0));
static CoordinateFrame headP(			normalIdToMatrix3(NORM_Y), Vector3(0, -0.5, 0));

static CoordinateFrame headTorsoOffset( neckP * headP.inverse());			// used to calculate camera target

static float defaultDisplayDistance(100.0f);

const char* const sHumanoid = "Humanoid";
const float epsilonDisplacement = 0.000001f;
const double epsilonDisplacementSqr = epsilonDisplacement * epsilonDisplacement;

REFLECTION_BEGIN();
// UI
static const Reflection::RefPropDescriptor<Humanoid, PartInstance> propTorso("Torso", category_Data, &Humanoid::getTorsoDangerous, &Humanoid::setTorso, Reflection::PropertyDescriptor::UI);
static const Reflection::RefPropDescriptor<Humanoid, PartInstance> propLeftLeg("LeftLeg", category_Data, &Humanoid::getLeftLegDangerous, &Humanoid::setLeftLeg, Reflection::PropertyDescriptor::UI);
static const Reflection::RefPropDescriptor<Humanoid, PartInstance> propRightLeg("RightLeg", category_Data, &Humanoid::getRightLegDangerous, &Humanoid::setRightLeg, Reflection::PropertyDescriptor::UI);


static const Reflection::PropDescriptor<Humanoid, float> propHealthUi("Health", "Game", &Humanoid::getHealth, &Humanoid::setHealthUi, Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<Humanoid, float> propHealth("Health_XML", "Game", &Humanoid::getHealth, &Humanoid::setHealth, Reflection::PropertyDescriptor::STREAMING);
static const Reflection::PropDescriptor<Humanoid, float> propMaxHealth("MaxHealth", "Game", &Humanoid::getMaxHealth, &Humanoid::setMaxHealth);
static const Reflection::PropDescriptor<Humanoid, float> propMaxHealthDep("maxHealth", "Game", &Humanoid::getMaxHealth, &Humanoid::setMaxHealth, Reflection::PropertyDescriptor::Attributes::deprecated(propMaxHealth));
static const Reflection::PropDescriptor<Humanoid, float> propWalkSpeed("WalkSpeed", "Game", &Humanoid::getWalkSpeed, &Humanoid::setWalkSpeed);

static const Reflection::PropDescriptor<Humanoid, float> propJumpPower("JumpPower", "Game", &Humanoid::getJumpPower, &Humanoid::setJumpPower);
static const Reflection::PropDescriptor<Humanoid, float> propMaxSlopeAngle("MaxSlopeAngle", "Game", &Humanoid::getMaxSlopeAngle, &Humanoid::setMaxSlopeAngle);
static const Reflection::PropDescriptor<Humanoid, float> propHipHeight("HipHeight", "Game", &Humanoid::getHipHeight, &Humanoid::setHipHeight);

// SCRIPTING
static Reflection::BoundFuncDesc<Humanoid, void(Vector3, bool)>    moveFunction(&Humanoid::move, "Move", "moveDirection","relativeToCamera", false, Security::None);

static const Reflection::RefPropDescriptor<Humanoid, PartInstance> propSeatPart("SeatPart", "Control", &Humanoid::getSeatPart, NULL, Reflection::PropertyDescriptor::SCRIPTING);
static const Reflection::PropDescriptor<Humanoid, Vector3> propLuaMoveDirection("MoveDirection", "Control", &Humanoid::getLuaMoveDirection, NULL, Reflection::PropertyDescriptor::SCRIPTING);
static const Reflection::RefPropDescriptor<Humanoid, PartInstance> propWalkToPart("WalkToPart", "Control", &Humanoid::getWalkToPart, &Humanoid::setWalkToPart, Reflection::PropertyDescriptor::SCRIPTING);
static const Reflection::PropDescriptor<Humanoid, Vector3> propWalkToPoint("WalkToPoint", "Control", &Humanoid::getWalkToPoint, &Humanoid::setWalkToPoint, Reflection::PropertyDescriptor::SCRIPTING);
static const Reflection::PropDescriptor<Humanoid, Vector3> propTargetPoint("TargetPoint", "Control", &Humanoid::getTargetPoint, &Humanoid::setTargetPointLocal, Reflection::PropertyDescriptor::SCRIPTING);
static const Reflection::PropDescriptor<Humanoid, bool> propJump("Jump", "Control", &Humanoid::getJump, &Humanoid::setJump, Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<Humanoid, bool> propJumpRep("JumpReplicate", "Control", &Humanoid::getJump, &Humanoid::setJump, Reflection::PropertyDescriptor::REPLICATE_ONLY);
static const Reflection::PropDescriptor<Humanoid, bool> propSit("Sit", "Control", &Humanoid::getSit, &Humanoid::setSit, Reflection::PropertyDescriptor::SCRIPTING);
static const Reflection::PropDescriptor<Humanoid, bool> propPlatformStanding("PlatformStand", "Control", &Humanoid::getPlatformStanding, &Humanoid::setPlatformStanding, Reflection::PropertyDescriptor::SCRIPTING);
static const Reflection::PropDescriptor<Humanoid, bool> propAutoRotate("AutoRotate", "Control", &Humanoid::getAutoRotate, &Humanoid::setAutoRotate, Reflection::PropertyDescriptor::SCRIPTING);
static const Reflection::PropDescriptor<Humanoid, bool> propAutoJumpEnabled("AutoJumpEnabled", "Control", &Humanoid::getAutoJumpEnabled, &Humanoid::setAutoJumpEnabled, Reflection::PropertyDescriptor::SCRIPTING);

// REPLICATE_ONLY

static const Reflection::PropDescriptor<Humanoid, Vector3> propWalkDirection("WalkDirection", "Control", &Humanoid::getWalkDirection, &Humanoid::setWalkDirection, Reflection::PropertyDescriptor::REPLICATE_ONLY);
static const Reflection::PropDescriptor<Humanoid, Vector3> propLuaMoveDirectionInternal("MoveDirectionInternal", "Control", &Humanoid::getLuaMoveDirection, &Humanoid::setLuaMoveDirection, Reflection::PropertyDescriptor::REPLICATE_ONLY);
static const Reflection::PropDescriptor<Humanoid, float> propWalkAngleError("WalkAngleError", "Control", &Humanoid::getWalkAngleError, &Humanoid::setWalkAngleError, Reflection::PropertyDescriptor::REPLICATE_ONLY);
static const Reflection::PropDescriptor<Humanoid, bool> propStrafe("Strafe", "Control", &Humanoid::getStrafe, &Humanoid::setStrafe, Reflection::PropertyDescriptor::REPLICATE_ONLY);

static Reflection::EnumPropDescriptor<Humanoid, Humanoid::HumanoidRigType> propRigType("RigType", category_Data, &Humanoid::getRigType, &Humanoid::setRigType, Reflection::PropertyDescriptor::SCRIPTING);
static const Reflection::PropDescriptor<Humanoid, float> propCameraMinDistance("CameraMinDistance", category_Data, &Humanoid::getMinDistance, &Humanoid::setMinDistance, Reflection::PropertyDescriptor::REPLICATE_ONLY);
static const Reflection::PropDescriptor<Humanoid, float> propCameraMaxDistance("CameraMaxDistance", category_Data, &Humanoid::getMaxDistance, &Humanoid::setMaxDistance, Reflection::PropertyDescriptor::REPLICATE_ONLY);
static const Reflection::PropDescriptor<Humanoid, Vector3> propCameraOffset("CameraOffset", category_Data, &Humanoid::getCamearaOffset, &Humanoid::setCameraOffset, Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::EnumPropDescriptor<Humanoid, RBX::Camera::CameraMode> prop_CameraMode("CameraMode", category_Data, &Humanoid::getCameraMode, &Humanoid::setCameraMode, Reflection::PropertyDescriptor::REPLICATE_ONLY);

static Reflection::BoundFuncDesc<Humanoid, void(float)> func_TakeDamage(&Humanoid::takeDamage, "TakeDamage", "amount", Security::None);
static Reflection::BoundFuncDesc<Humanoid, void(float)> dep_TakeDamage(&Humanoid::takeDamage, "takeDamage", "amount", Security::None, Reflection::Descriptor::Attributes::deprecated(func_TakeDamage));
static Reflection::BoundFuncDesc<Humanoid, void(bool)> func_SetClickToWalkEnabled(&Humanoid::setClickToWalkEnabled, "SetClickToWalkEnabled","enabled", Security::RobloxScript);
static Reflection::BoundFuncDesc<Humanoid, void(Vector3, shared_ptr<Instance>)> func_MoveTo(&Humanoid::moveTo2, "MoveTo", "location", "part", shared_ptr<Instance>(), Security::None);
static Reflection::EventDesc<Humanoid, void(bool)> event_MoveToFinished(&Humanoid::moveToFinishedSignal, "MoveToFinished", "reached");

// These now fire client and server side, to allow for local animation
static Reflection::EventDesc<Humanoid, void()> event_Died(&Humanoid::diedSignal, "Died");
static Reflection::EventDesc<Humanoid, void(float)> event_Running(&Humanoid::runningSignal, "Running", "speed");
static Reflection::EventDesc<Humanoid, void(float)> event_Climbing(&Humanoid::climbingSignal, "Climbing", "speed");
static Reflection::EventDesc<Humanoid, void(bool)> event_Jumping(&Humanoid::jumpingSignal, "Jumping", "active");
static Reflection::EventDesc<Humanoid, void(bool)> event_FreeFalling(&Humanoid::freeFallingSignal, "FreeFalling", "active");
static Reflection::EventDesc<Humanoid, void(bool)> event_GettingUp(&Humanoid::gettingUpSignal, "GettingUp", "active");
static Reflection::EventDesc<Humanoid, void(bool)> event_Strafing(&Humanoid::strafingSignal, "Strafing", "active");
static Reflection::EventDesc<Humanoid, void(bool)> event_FallingDown(&Humanoid::fallingDownSignal, "FallingDown", "active");
static Reflection::EventDesc<Humanoid, void(bool)> event_Ragdoll(&Humanoid::ragdollSignal, "Ragdoll", "active");
static Reflection::EventDesc<Humanoid, void(bool, shared_ptr<Instance>)> event_Seated(&Humanoid::seatedSignal, "Seated", "active", "currentSeatPart");
static Reflection::EventDesc<Humanoid, void(bool)> event_PlatformStanding(&Humanoid::platformStandingSignal, "PlatformStanding", "active");
static Reflection::EventDesc<Humanoid, void(float)> event_Swimming(&Humanoid::swimmingSignal, "Swimming", "speed");
    static Reflection::EventDesc<Humanoid, void(HUMAN::StateType, HUMAN::StateType)> event_StateChanged(&Humanoid::stateChangedSignal, "StateChanged", "old", "new");
static Reflection::BoundFuncDesc<Humanoid, HUMAN::StateType()> func_GetState(&Humanoid::getCurrentStateType, "GetState", Security::None);
static Reflection::BoundFuncDesc<Humanoid, void(HUMAN::StateType)> func_ChangeState(&Humanoid::changeState, "ChangeState", "state", RBX::HUMAN::xx, Security::None);

static Reflection::EventDesc<Humanoid, void(float)> event_HealthChanged(&Humanoid::healthChangedSignal, "HealthChanged", "health");

static Reflection::BoundFuncDesc<Humanoid, bool(Humanoid::Status)> func_AddStatus(&Humanoid::addStatus, "AddStatus", "status", Humanoid::POISON_STATUS, Security::None, Reflection::Descriptor::Attributes::deprecated());
static Reflection::BoundFuncDesc<Humanoid, bool(Humanoid::Status)> func_RemoveStatus(&Humanoid::removeStatus, "RemoveStatus", "status", Humanoid::POISON_STATUS, Security::None, Reflection::Descriptor::Attributes::deprecated());
static Reflection::BoundFuncDesc<Humanoid, bool(Humanoid::Status)> func_HasStatus(&Humanoid::hasStatus, "HasStatus", "status", Humanoid::POISON_STATUS, Security::None, Reflection::Descriptor::Attributes::deprecated());

static Reflection::BoundFuncDesc<Humanoid, bool(std::string)> func_AddCustomStatus(&Humanoid::addCustomStatus, "AddCustomStatus", "status", Security::None, Reflection::Descriptor::Attributes::deprecated());
static Reflection::BoundFuncDesc<Humanoid, bool(std::string)> func_RemoveCustomStatus(&Humanoid::removeCustomStatus, "RemoveCustomStatus", "status", Security::None, Reflection::Descriptor::Attributes::deprecated());
static Reflection::BoundFuncDesc<Humanoid, bool(std::string)> func_HasCustomStatus(&Humanoid::hasCustomStatus, "HasCustomStatus", "status", Security::None, Reflection::Descriptor::Attributes::deprecated());

static Reflection::BoundFuncDesc<Humanoid, void(shared_ptr<Instance>)> func_equipTool(&Humanoid::equipToolInstance, "EquipTool", "tool", Security::None);
static Reflection::BoundFuncDesc<Humanoid, void()> func_unequipTools(&Humanoid::unequipTools, "UnequipTools", Security::None);

static Reflection::BoundFuncDesc<Humanoid, shared_ptr<const Reflection::ValueArray>()>  func_getStatuses(&Humanoid::getStatuses, "GetStatuses", Security::None, Reflection::Descriptor::Attributes::deprecated());

static Reflection::EventDesc<Humanoid, void(Humanoid::Status)> event_StatusAdded(&Humanoid::statusAddedSignal, "StatusAdded", "status", Reflection::Descriptor::Attributes::deprecated());
static Reflection::EventDesc<Humanoid, void(Humanoid::Status)> event_StatusRemoved(&Humanoid::statusRemovedSignal, "StatusRemoved", "status", Reflection::Descriptor::Attributes::deprecated());

static Reflection::EventDesc<Humanoid, void(std::string)> event_CustomStatusAdded(&Humanoid::customStatusAddedSignal, "CustomStatusAdded", "status", Reflection::Descriptor::Attributes::deprecated());
static Reflection::EventDesc<Humanoid, void(std::string)> event_CustomStatusRemoved(&Humanoid::customStatusRemovedSignal, "CustomStatusRemoved", "status", Reflection::Descriptor::Attributes::deprecated());

static Reflection::RemoteEventDesc<Humanoid, void(shared_ptr<Instance>)> desc_serverEquipTool(&Humanoid::serverEquipToolSignal, "ServerEquipTool", "tool", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);

static Reflection::EnumPropDescriptor<Humanoid, Humanoid::NameOcclusion> prop_NameOcclusion("NameOcclusion", category_Data, &Humanoid::getNameOcclusion, &Humanoid::setNameOcclusion);
static Reflection::EnumPropDescriptor<Humanoid, Humanoid::HumanoidDisplayDistanceType> prop_DisplayDistanceType("DisplayDistanceType", category_Data, &Humanoid::getDisplayDistanceType, &Humanoid::setDisplayDistanceType);
static const Reflection::PropDescriptor<Humanoid, float> propNameDisplayDistance("NameDisplayDistance", category_Data, &Humanoid::getNameDisplayDistance, &Humanoid::setNameDisplayDistance);
static const Reflection::PropDescriptor<Humanoid, float> propHealthDisplayDistance("HealthDisplayDistance", category_Data, &Humanoid::getHealthDisplayDistance, &Humanoid::setHealthDisplayDistance);

static Reflection::BoundFuncDesc<Humanoid, bool(HUMAN::StateType)> desc_getStateTransitionEnabled(&Humanoid::getStateTransitionEnabled, "GetStateEnabled","state", Security::None);
    static Reflection::BoundFuncDesc<Humanoid, void(HUMAN::StateType, bool)> desc_setStateTransitionEnabled(&Humanoid::setStateTransitionEnabled, "SetStateEnabled","state", "enabled", Security::None);
    static Reflection::EventDesc<Humanoid, void(HUMAN::StateType, bool)> event_StateEnabledChanged(&Humanoid::stateEnabledChangedSignal, "StateEnabledChanged", "state", "isEnabled");

//keep these in sync with the Animator object.
// this is proxy interface to the Animator that is contained in Humanoid.
static Reflection::BoundFuncDesc<Humanoid, shared_ptr<Instance>(shared_ptr<Instance>)> desc_LoadAnimation(&Humanoid::loadAnimation, "LoadAnimation","animation", Security::None);
static Reflection::BoundFuncDesc<Humanoid, shared_ptr<Instance>(shared_ptr<Instance>)> dep_loadAnimation(&Humanoid::loadAnimation, "loadAnimation","animation", Security::None, Reflection::Descriptor::Attributes::deprecated(desc_LoadAnimation));
static Reflection::BoundFuncDesc<Humanoid, shared_ptr<const Reflection::ValueArray>()> desc_GetPlayingAnimationTracks(&Humanoid::getPlayingAnimationTracks, "GetPlayingAnimationTracks", Security::None);

static Reflection::EventDesc<Humanoid, void(shared_ptr<Instance>)> event_AnimationPlayed(&Humanoid::animationPlayedSignal, "AnimationPlayed", "animationTrack");
REFLECTION_END();


namespace Reflection {
template<>
EnumDesc<HUMAN::StateType>::EnumDesc()
	:EnumDescriptor("HumanoidStateType")
{
	addPair(RBX::HUMAN::FALLING_DWN, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::FALLING_DWN));
	addPair(RBX::HUMAN::RUNNING, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::RUNNING));
	addPair(RBX::HUMAN::RUNNING_NO_PHYS, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::RUNNING_NO_PHYS));
	addPair(RBX::HUMAN::CLIMBING, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::CLIMBING));
	addPair(RBX::HUMAN::STRAFING_NO_PHYS, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::STRAFING_NO_PHYS));
	addPair(RBX::HUMAN::RAGDOLL, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::RAGDOLL));
	addPair(RBX::HUMAN::GETTING_UP, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::GETTING_UP));
	addPair(RBX::HUMAN::JUMPING, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::JUMPING));
	addPair(RBX::HUMAN::LANDED, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::LANDED));
	addPair(RBX::HUMAN::FLYING, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::FLYING));
	addPair(RBX::HUMAN::FREE_FALL, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::FREE_FALL));
	addPair(RBX::HUMAN::SEATED, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::SEATED));
	addPair(RBX::HUMAN::PLATFORM_STANDING, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::PLATFORM_STANDING));
	addPair(RBX::HUMAN::DEAD, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::DEAD));
	addPair(RBX::HUMAN::SWIMMING, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::SWIMMING));
	addPair(RBX::HUMAN::PHYSICS, RBX::HUMAN::HumanoidState::getStateNameByType(RBX::HUMAN::PHYSICS));
	addPair(RBX::HUMAN::xx, "None");
}
template<>
HUMAN::StateType& Variant::convert<HUMAN::StateType>(void)
{
	return genericConvert<HUMAN::StateType>();
}

}

bool Humanoid::isStateInString(const std::string& text, const HUMAN::StateType &compare, HUMAN::StateType& value)
{
	if(text == RBX::HUMAN::HumanoidState::getStateNameByType(compare)) {
		value = compare;
		return true;
	}
	return false;
}

template<>
bool RBX::StringConverter<HUMAN::StateType>::convertToValue(const std::string& text, HUMAN::StateType& value)
{
	if (Humanoid::isStateInString(text, RBX::HUMAN::FALLING_DWN, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::RUNNING, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::RUNNING_NO_PHYS, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::CLIMBING, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::STRAFING_NO_PHYS, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::RAGDOLL, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::GETTING_UP, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::JUMPING, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::LANDED, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::FLYING, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::FREE_FALL, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::SEATED, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::PLATFORM_STANDING, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::DEAD, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::SWIMMING, value)) return true;
	if (Humanoid::isStateInString(text, RBX::HUMAN::PHYSICS, value)) return true;
	if(text.find("None")){
		value = RBX::HUMAN::xx;
		return true;
	}
	return false;
}

static const std::string kAppendageNames[] =
{
    "HumanoidRootPart", "Head", "Right Arm", "Left Arm", "Right Leg", "Left Leg", "Torso"
};

static const std::string  kAppendageNamesR15[] =
{
	"HumanoidRootPart", "Head", "RightArm", "LeftArm", "RightLeg", "LeftLeg", "UpperTorso"
};

static const std::string& getAppendageString(size_t appendage, bool R15)
{
    if (appendage < sizeof(kAppendageNames) / sizeof(kAppendageNames[0]))
	{
		if (R15)
			return kAppendageNamesR15[appendage];
		else
	        return kAppendageNames[appendage];
	}
    else
    {
        RBXASSERT(!"Unknown appendage type");

        static std::string dummy;
        return dummy;
    }
}

Humanoid::Humanoid()
:world(NULL)
, torsoArrived(false)
, localSimulating(false)
, ownedByLocalPlayer(false)
, walkAngleError(0.0f)
, walkSpeed(16.0f)
, walkSpeedShadow(16.0f)
, percentWalkSpeed(1.0f)
, walkSpeedErrors(0)
, jump(false)
, autoJump(false) 
, sit(false)
, jumpPower(HUMAN::Jumping::kJumpVelocityGrid())
, maxSlopeAngle(89.0)
, hipHeight(0.0)
, touchedHard(false)
, activatePhysics(false)
, ragdollCriteria(34)
, numContacts(0)
, platformStanding(false)
, strafe(false)
, health(100.0f)
, maxHealth(100.0f)
, nameDisplayDistance(defaultDisplayDistance)
, healthDisplayDistance(defaultDisplayDistance)
, hadNeck(false)
, hadHealth(false)
, isWalking(false)
, walkTimer(0)
, displayText("")
, filterState(ContentFilter::Waiting)
, clickToWalkEnabled(true)
, typing(false)
, nameOcclusion(Humanoid::NAME_OCCLUSION_NONE)
, displayDistanceType(Humanoid::HUMANOID_DISPLAY_DISTANCE_TYPE_VIEWER)
, autorotate(true)
, luaMoveDirection(Vector3::zero())
, isWalkingFromStudioTouchEmulation(false)
, lastFloorNormal(Vector3::unitY())
, lastFloorPart()
, rootFloorMechPart()
, lastFilterPhase(0)
, autoJumpEnabled(true)
, previousState(HUMAN::FREE_FALL)
, rigType(HUMANOID_RIG_TYPE_R6)
, pos0()
, pos1()
, pos2()
{
	setName("Humanoid");

	FASTLOG1(FLog::ISteppedLifetime, "Humanoid created - %p", this);
    for (int i = HUMAN::FALLING_DWN; i < HUMAN::NUM_STATE_TYPES; i++)
    {
        stateTransitionEnabled[i] = true;
    }

	if (DFFlag::NamesOccludedAsDefault)
	{
		nameOcclusion = Humanoid::NAME_OCCLUSION_ALL;
	}
}

Humanoid::~Humanoid()
{
	RBXASSERT(!currentState.get());
	RBXASSERT(world==NULL);
	FASTLOG1(FLog::ISteppedLifetime, "Humanoid destroyed - %p", this);
}

void collectStatus(shared_ptr<RBX::Instance> instance, Reflection::ValueArray* result)
{
	//Try to push an enum first
	if(const Reflection::EnumDescriptor::Item* item = Reflection::EnumDesc<Humanoid::Status>::singleton().lookup(instance->getName().c_str())){
		Reflection::Variant value;
		if(item->convertToValue(value)){
			result->push_back(value);
			return;
		}
	}

	//But if that doesn't work, push a string
	result->push_back(instance->getName());
}

shared_ptr<const Reflection::ValueArray> Humanoid::getStatuses()
{
	shared_ptr<Reflection::ValueArray> result(rbx::make_shared<Reflection::ValueArray>());
	if(StatusInstance* status = getStatusFast()){
		status->visitChildren(boost::bind(&collectStatus, _1, result.get()));
	}
	return result;
}

bool Humanoid::hasStatus(Status newStatus)
{
	return hasCustomStatus(Reflection::EnumDesc<Humanoid::Status>::singleton().convertToString(newStatus));
}

bool Humanoid::hasCustomStatus(std::string name)
{
	if(StatusInstance* status = getStatusFast()){
		if(status->findFirstChildByName(name)){
			return true;
		}
		return false;
	}
	return false;
}
bool Humanoid::addCustomStatus(std::string name)
{
	if(StatusInstance* status = getStatusFast()){
		if(status->findFirstChildByName(name)){
			//Already exists
			return false;
		}

		shared_ptr<BoolValue> boolValue = Creatable<Instance>::create<BoolValue>();
		boolValue->setName(name);
		boolValue->setValue(true); //don't care
		boolValue->setParent(getStatusFast());
		return true;
	}
	return false;
}

bool Humanoid::addStatus(Status newStatus)
{
	return addCustomStatus(Reflection::EnumDesc<Humanoid::Status>::singleton().convertToString(newStatus));
}

bool Humanoid::removeCustomStatus(std::string name)
{
	if(StatusInstance* status = getStatusFast()){
		if(Instance* instance = status->findFirstChildByName(name))
		{
			instance->setParent(NULL);
			return true;
		}

		return false;
	}
	return false;
}

bool Humanoid::removeStatus(Status newStatus)
{
	return removeCustomStatus(Reflection::EnumDesc<Humanoid::Status>::singleton().convertToString(newStatus));
}

void Humanoid::setNameOcclusion(NameOcclusion value)
{
	if(value != nameOcclusion)
	{
		nameOcclusion = value;
		raisePropertyChanged(prop_NameOcclusion);
	}
}

void Humanoid::setDisplayDistanceType(HumanoidDisplayDistanceType value)
{
	if (value != displayDistanceType)
	{
		displayDistanceType = value;
		raisePropertyChanged(prop_DisplayDistanceType);
	}
}

void Humanoid::onDescendantAdded(Instance* instance)
{
	if(instance->getParent() == getStatusFast()){
		Status status;
		if(Reflection::EnumDesc<Humanoid::Status>::singleton().convertToValue(instance->getName().c_str(), status)){
			statusAddedSignal(status);
		}
		customStatusAddedSignal(instance->getName());
	}
	Super::onDescendantAdded(instance);
}
void Humanoid::onDescendantRemoving(const shared_ptr<Instance>& instance)
{
	if(instance->getParent() == getStatusFast()){
		Status status;
		if(Reflection::EnumDesc<Humanoid::Status>::singleton().convertToValue(instance->getName().c_str(), status)){
			statusRemovedSignal(status);
		}
		customStatusRemovedSignal(instance->getName());
	}
	Super::onDescendantRemoving(instance);	
}

void Humanoid::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if(newProvider)
	{
		humanoidEquipConnection	= serverEquipToolSignal.connect(boost::bind(&Humanoid::equipToolInstance,this,_1));

		if (DFFlag::UseR15Character && RBX::Network::Players::backendProcessing(this))
		{
			DataModel* dataModel = Instance::fastDynamicCast<DataModel>(newProvider);
			if (dataModel)
			{
				RBX::Security::Impersonator impersonate(RBX::Security::Replicator_);
				if (dataModel->getForceR15())
					setRigType(Humanoid::HUMANOID_RIG_TYPE_R15);
				else 
					setRigType(Humanoid::HUMANOID_RIG_TYPE_R6);
			}
		}
	}
	else if(oldProvider)
	{
		humanoidEquipConnection.disconnect();
	}

	Super::onServiceProvider(oldProvider, newProvider);

	// IStepped hook
	onServiceProviderIStepped(oldProvider, newProvider);

	ServiceProvider::create< FilteredSelection<PartInstance> >(newProvider);		// will need this later on during const access


}


bool Humanoid::getDead() const
{
	if (this->world != NULL) {
		RBXASSERT(currentState.get());
		return (currentState->getStateType() == RBX::HUMAN::DEAD);
	}
	else {
		return true;
	}
}

void Humanoid::setWalkSpeed(float value)
{
	if (DFFlag::HumanoidCheckForNegatives && value < 0.0f)
		value = 0.0f;

	if (value != walkSpeed)
	{
		walkSpeed = value;
		walkSpeedShadow = value;
		raisePropertyChanged(propWalkSpeed);
	}
}
    
void Humanoid::setPercentWalkSpeed(float value)
{
    value = G3D::clamp(value,0.0f,1.0f);
    
    if(value != percentWalkSpeed)
    {
        percentWalkSpeed = value;
    }
}

void Humanoid::setJumpPower(float value)
{
	value = G3D::clamp(value,0.0f,1000.0f);

	if(value != jumpPower)
	{
		jumpPower = value;
		raisePropertyChanged(propJumpPower);
	}
}


void Humanoid::setMaxSlopeAngle(float value)
{
	value = G3D::clamp(value,0.0f,89.9f);

	if(value != maxSlopeAngle)
	{
		maxSlopeAngle = value;
		raisePropertyChanged(propMaxSlopeAngle);
	}
}

void Humanoid::setHipHeight(float value)
{
	if(value != hipHeight)
	{
		hipHeight = value;
		raisePropertyChanged(propHipHeight);
	}
}

void Humanoid::setHealth(float value)
{
	if (health != value)
	{
		health = value;
		updateHadHealth();
		raisePropertyChanged(propHealth);
		healthChangedSignal(value);
	}
}

void Humanoid::setHealthUi(float value)
{
	float legalValue = G3D::clamp(value, 0.0f, maxHealth);

	if (health != legalValue)
	{
		raisePropertyChanged(propHealthUi);
		setHealth(legalValue);
	}	
	else if(legalValue != value)
	{
		raisePropertyChanged(propHealthUi);
	}
}


void Humanoid::setMaxHealth(float value)
{
	if (DFFlag::HumanoidCheckForNegatives && value < 0.0f)
		value = 0.0f;

	if (maxHealth != value)
	{
		maxHealth = value;
		raisePropertyChanged(propMaxHealth);
		healthChangedSignal(health);
	}
}


void Humanoid::takeDamage(float value)		// honors explosions, forceFields, etc.
{
	RBX::PartInstance* tempTorso = getTorsoSlow();
	if (tempTorso) {
		if (!ForceField::partInForceField(tempTorso)) {
			setHealth(getHealth() - value);
		}
	}
}

const Humanoid* Humanoid::getConstLocalHumanoidFromContext(const Instance* context)
{
	if (const ModelInstance* character = Network::Players::findConstLocalCharacter(context)) {
		return modelIsConstCharacter(character);
	}
	else {
		return NULL;
	}
}

Humanoid* Humanoid::getLocalHumanoidFromContext(Instance* context)
{
	if (ModelInstance* character = Network::Players::findLocalCharacter(context)) {
		return modelIsCharacter(character);
	}
	else {
		return NULL;
	}
}

PartInstance* Humanoid::getLocalHeadFromContext(Instance* context)
{
	ModelInstance* character = Network::Players::findLocalCharacter(context);
	return getHeadFromCharacter(character);
}

const PartInstance* Humanoid::getConstLocalHeadFromContext(const Instance* context)
{
	const ModelInstance* character = Network::Players::findConstLocalCharacter(context);
	return getConstHeadFromCharacter(character);
}


const PartInstance* Humanoid::getConstHeadFromCharacter(const ModelInstance* character)
{
	return (character) 
				? Instance::fastDynamicCast<PartInstance>(character->findConstFirstChildByName("Head"))
				: NULL;
}

PartInstance* Humanoid::getHeadFromCharacter(ModelInstance* character)
{
	return const_cast<PartInstance*>(getConstHeadFromCharacter(character));
}


Weld* Humanoid::getGrip(Instance* character)
{
	if (Humanoid* humanoid = modelIsCharacter(character))
	{
		if (PartInstance* right_arm = humanoid->getRightArmSlow())
		{
			return Instance::fastDynamicCast<Weld>(right_arm->findFirstChildByName("RightGrip"));
		}
	}
	return NULL;
}

shared_ptr<JointInstance> newJoint(bool animated)
{
	if (animated) {
		shared_ptr<Motor> m = Creatable<Instance>::create<Motor6D>();
		m->setMaxVelocity(0.1f);
		return m;
	}
	else {
		shared_ptr<Snap> s = Creatable<Instance>::create<Snap>();
		return s;
	}
}

void Humanoid::setRigType(HumanoidRigType type)
{ 
	try {
		RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "setRigType");
	} 
	catch (RBX::base_exception& e) 
	{
		raisePropertyChanged(propRigType);
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficient permissions to setRigType");
		throw e;
	}

	if (type != rigType)
	{
		rigType = type; 
		raisePropertyChanged(propRigType);

		if (getParent())
		{
			// update all cache entries
			for (int i = TORSO; i < APPENDAGE_COUNT; i++)
			{
				appendageCache[i] = Instance::fastSharedDynamicCast<PartInstance>(shared_from(getParent()->findFirstChildByName(getAppendageString(i, getUseR15()))));
				updateSiblingPropertyListener(appendageCache[i]);
			}
			updateBaseInstance();

			// register update to listen to sibling changes
			characterChildAdded = getParent()->onDemandWrite()->childAddedSignal.connect(boost::bind(&Humanoid::onEvent_ChildModified, this, _1));
			characterChildRemoved = getParent()->onDemandWrite()->childRemovedSignal.connect(boost::bind(&Humanoid::onEvent_ChildModified, this, _1));
		}

	}
}

static void createJoint(const std::string& jointName, Attachment& parentAttachment, Attachment& partForJointAttachment)
{
	shared_ptr<Motor6D> newMotor = Creatable<Instance>::create<Motor6D>();
	PartInstance* partForJoint = Instance::fastDynamicCast<PartInstance>(partForJointAttachment.getParent());

	newMotor->setName(jointName);

	newMotor->setPart0(Instance::fastDynamicCast<PartInstance>(parentAttachment.getParent()));
	newMotor->setPart1(partForJoint);

	newMotor->setC0(parentAttachment.getFrameInPart());
	newMotor->setC1(partForJointAttachment.getFrameInPart());

	newMotor->setParent(partForJoint);
}

void Humanoid::buildJointsFromAttachments(PartInstance* part, std::vector<PartInstance*>& characterParts)
{
	if (part == NULL)
	{
		return;
	}

	if (shared_ptr<const Instances> children = part->getChildren2())
	{
		// first, loop thru all of the part's children to find attachments
		Instances::const_iterator end =children->end();
		for (Instances::const_iterator iter = children->begin(); iter != end; ++iter)
		{
			if(RBX::Attachment* attachment = Instance::fastDynamicCast<RBX::Attachment>((*iter).get()))
			{
				// only do joint build from "RigAttachments"
				const std::string attachmentName = attachment->getName();
				size_t findPos = attachmentName.find("RigAttachment");
				if (findPos == std::string::npos)
				{
					continue;
				}
				// also don't make double joints (there is the same named
				// rigattachment under two parts)
				const std::string jointName = attachmentName.substr(0, findPos);
				if (part->findConstFirstChildByName(jointName))
				{
					continue;
				}

				// try to find other part with same rig attachment name
				for (std::vector<PartInstance*>::const_iterator partIter = characterParts.begin(); partIter != characterParts.end(); ++partIter)
				{
					PartInstance* characterPart = *partIter;
					if (characterPart != part)
					{
						if (Instance* instance = characterPart->findFirstChildByName(attachmentName))
						{
							if (Attachment* matchingAttachment = Instance::fastDynamicCast<Attachment>(instance))
							{
								createJoint(jointName, *attachment, *matchingAttachment);
								buildJointsFromAttachments(characterPart, characterParts);
								break;
							}
						}
					}
				}
			}
		}
	}
}

// Server side
void Humanoid::buildJoints(RBX::DataModel* dm)
{
	if(!status){
		status = Creatable<Instance>::create<RBX::StatusInstance>();
		Instance::propArchivable.setValue(status.get(), false);
		status->setParent(this);
	}

	ModelInstance* character = getCharacterFromHumanoid(this);
	RBXASSERT(character);
	if (character)
	{	
		if (dm && dm->getForceR15())
		{
			PartInstance* torso = getTorsoFast();
			if (torso)
			{
				std::vector<PartInstance*> characterParts;
				getParts(characterParts);

				buildJointsFromAttachments(torso, characterParts);
			}
		}
		else
		{
			PartInstance* head = getHeadSlow();
			PartInstance* torso = getAppendageSlow(VISIBLE_TORSO);
			PartInstance* rightArm = getRightArmSlow();
			PartInstance* leftArm = getLeftArmSlow();
			PartInstance* rightLeg = getRightLegSlow();
			PartInstance* leftLeg = getLeftLegSlow();

			RBXASSERT(head && torso && rightArm && leftArm && rightLeg && leftLeg);

			if (torso)
			{
				PartInstance *pRoot = getTorsoSlow();

				if (pRoot != NULL)
				{
					shared_ptr<JointInstance> rootJoint = newJoint(true);
					rootJoint->setName("RootJoint");
					rootJoint->setPart0(pRoot);
					rootJoint->setPart1(torso);

					CoordinateFrame torsoP(			normalIdToMatrix3(NORM_Y), Vector3(0, 0, 0));
					rootJoint->setC0(torsoP);
					rootJoint->setC1(torsoP);

					rootJoint->setParent(pRoot);

					pRoot->getPartPrimitive()->setMassInertia(0.1f);  // Should be small, but not zero.  Physics doesn't like 0 weight parts
				}

				shared_ptr<JointInstance> rightShoulder = newJoint(true);
				shared_ptr<JointInstance> leftShoulder = newJoint(true);
				shared_ptr<JointInstance> rightHip = newJoint(true);
				shared_ptr<JointInstance> leftHip = newJoint(true);
				shared_ptr<JointInstance> neck = newJoint(true);

				rightShoulder->setName("Right Shoulder");
				leftShoulder->setName("Left Shoulder");
				rightHip->setName("Right Hip");
				leftHip->setName("Left Hip");
				neck->setName("Neck");

				rightShoulder->setPart0(torso);
				leftShoulder->setPart0(torso);
				rightHip->setPart0(torso);
				leftHip->setPart0(torso);
				neck->setPart0(torso);

				rightShoulder->setPart1(rightArm);
				leftShoulder->setPart1(leftArm);
				rightHip->setPart1(rightLeg);
				leftHip->setPart1(leftLeg);
				neck->setPart1(head);


				rightShoulder->setC0(rightShoulderP);
				leftShoulder->setC0(leftShoulderP);
				rightHip->setC0(rightHipP);
				leftHip->setC0(leftHipP);
				neck->setC0(neckP);

				rightShoulder->setC1(rightArmP);
				leftShoulder->setC1(leftArmP);
				rightHip->setC1(rightLegP);
				leftHip->setC1(leftLegP);
				neck->setC1(headP);

				rightShoulder->setParent(torso);
				leftShoulder->setParent(torso);
				rightHip->setParent(torso);
				leftHip->setParent(torso);
				neck->setParent(torso);
			}
		}
	}
}




bool Humanoid::askSetParent(const Instance* instance) const {
	// Humanoids must be children of a ModelInstance
	// TODO: If we refactor Humanoind to BE a ModelInstance then this code can be removed
	return Instance::fastDynamicCast<ModelInstance>(instance)!=NULL;
}

const Humanoid* Humanoid::constHumanoidFromBodyPart(const Instance* bodyPart)
{
	return bodyPart ? modelIsConstCharacter(bodyPart->getParent()) : NULL;
}

const Humanoid* Humanoid::constHumanoidFromDescendant(const Instance* bodyPart)
{
	if (bodyPart) {
		const Instance *pParent = bodyPart->getParent();
		if (pParent != NULL)
		{
			const Humanoid *pHumanoid = modelIsConstCharacter(pParent);
			if (pHumanoid != NULL)
				return pHumanoid;
			else
				return constHumanoidFromDescendant(pParent);
		} 
	}

	return NULL;
}


Humanoid* Humanoid::humanoidFromBodyPart(Instance* bodyPart)
{
//	return bodyPart ? modelIsCharacter(bodyPart->getParent()) : NULL;
	return const_cast<Humanoid*>(constHumanoidFromBodyPart(bodyPart));
}


// abstract so as not to include Humanoid in general code
// for now - anything with a humanoid directly below it comes up yes
const Humanoid* Humanoid::modelIsConstCharacter(const Instance* testModel)
{
	return ModelInstance::findConstFirstModifierOfType<Humanoid>(testModel);
}

Humanoid* Humanoid::modelIsCharacter(Instance* testModel)
{
	return const_cast<Humanoid*>(modelIsConstCharacter(testModel));
}



const ModelInstance* Humanoid::getConstCharacterFromHumanoid(const Humanoid* humanoid)
{
	if (humanoid) {
		const Instance* parent = humanoid->getParent();
		if (const ModelInstance* m = Instance::fastDynamicCast<ModelInstance>(parent)) {
			return m;
		}
	}
	return NULL;
}

ModelInstance* Humanoid::getCharacterFromHumanoid(Humanoid* humanoid)
{
	return const_cast<ModelInstance*>(getConstCharacterFromHumanoid(humanoid));
}

///////////////////////////////////////////////////////////////
//
// Controller stuff


bool Humanoid::hasWalkToPoint(Vector3& worldPosition) const
{
	Network::Player* player = Network::Players::getPlayerFromCharacter(getCharacterFromHumanoid(const_cast<Humanoid*>(this))); // this should always be safe, right?
	// only do click-to-walk in certain modes
	if(player && (RBX::GameBasicSettings::singleton().inHybridMode() || RBX::GameBasicSettings::singleton().inMousepanMode()) )
		return false;

	if (walkToPart.get())
	{
		if (const Workspace* workspace = Workspace::findConstWorkspace(this)) 
		{
			if (workspace->contextInWorkspace(walkToPart.get()))
			{
				worldPosition = walkToPart->getCoordinateFrame().pointToWorldSpace(walkToPoint);
				return true;
			}
		}
	}
    else
    {
        worldPosition = walkToPoint;
        return true;
    }
	return false;
}

HUMAN::StateType Humanoid::getCurrentStateType()
{
	if (currentState.get())	
	{
		return currentState->getStateType();
	}
	return RBX::HUMAN::xx;
}

void Humanoid::setPreviousStateType(RBX::HUMAN::StateType newState)
{
	if (newState != previousState)
	{
		previousState = newState;
	}
}

// cds: This function is an exploit target because it can give several powers when abused.
void Humanoid::changeState(HUMAN::StateType state)
{
	if (currentState.get())	
	{
		currentState->setLuaState(state);
	}
    void (Humanoid::* thisFunction)(HUMAN::StateType) = &Humanoid::changeState;
    checkRbxCaller<kCallCheckCodeOnly, callCheckSetApiFlag<kChangeStateApiOffset> >(
        reinterpret_cast<void*>((void*&)(thisFunction)));
}


void Humanoid::equipToolInstance(shared_ptr<Instance> instance)
{
	if(instance)
		if(Tool* tool = Instance::fastDynamicCast<Tool>(instance.get()))
			equipTool(tool);
}

void Humanoid::equipTool(RBX::Tool* tool)
{
	if(tool)
	{
		if(ModelInstance* character = getCharacterFromHumanoid(this))
		{
			unequipTools();

			if (RBX::Network::Players::backendProcessing(this))
			{
				// if also frontend, we are in solo play
				if (Network::Players::frontendProcessing(this))
				{
					tool->setParent(character);
					return;
				}

				if(ModelInstance* character = getCharacterFromHumanoid(this))
				{
					Workspace* workspace = ServiceProvider::find<Workspace>(this);
					bool fromWorkspace = (tool->getParent() == workspace || Workspace::contextInWorkspace(tool->getParent()));

					// server signals the client that this humanoid belongs to
					Network::Player* player = Network::Players::getPlayerFromCharacter(character);
					if(player && !fromWorkspace) 
					{
						Reflection::EventArguments args(1);
						args[0] = Instance::fastSharedDynamicCast<Instance>(shared_from(tool));
						const RBX::SystemAddress addr = player->getRemoteAddressAsRbxAddress();
						raiseEventInvocation(desc_serverEquipTool, args, &addr);
					}
					else
					{
						// if this is not a player character or the tool is currenting under workspace, do equip on server
						tool->setParent(character);
					}
				}
			}
			else
			{
				// client can perform this only if the tool was not in workspace, otherwise let server do it
				Workspace* workspace = ServiceProvider::find<Workspace>(this);
				if (tool->getParent() != workspace && !Workspace::contextInWorkspace(tool->getParent()))
					tool->setParent(character);
				else
					desc_serverEquipTool.replicateEvent(this, shared_from(tool));
			}
		}
	}
}

void Humanoid::unequipTools()
{
	if(ModelInstance* character = getCharacterFromHumanoid(this))
    {
		if(character->getChildren())
        {
			if(Network::Player* player = Network::Players::getPlayerFromCharacter(character))
			{
				if(RBX::Backpack* backpack = player->findFirstChildOfType<Backpack>())
				{
					Instances::const_iterator end = character->getChildren()->end();
					for (Instances::const_iterator iter = character->getChildren()->begin(); iter != end; ++iter)
					{
						if(RBX::Tool* tool = Instance::fastDynamicCast<RBX::Tool>((*iter).get()))
							tool->setParent(backpack);
					}
				}
			}
			else
				StandardOut::singleton()->printf(MESSAGE_WARNING, "UnequipTools can only be called on a Humanoid with a corresponding Player object.");
        }
    }
}

Velocity Humanoid::calcDesiredWalkVelocity() const
{
	Velocity answer;
	Vector3 intendedMovementVector = walkDirection;

	// if we aren't currently trying to move somewhere via user input, set to override movement
	if (intendedMovementVector == Vector3::zero())
	{
		intendedMovementVector = luaMoveDirection;
		if (luaMoveDirection != Vector3::zero() && !luaMoveDirection.isUnit())
		{
			RBXASSERT(false);
		}
	}
	else
	{
		if (!allow3dWalkDirection()) {
			intendedMovementVector.y = 0;
		}
		if (!(intendedMovementVector == Vector3::zero())) {
			intendedMovementVector = Math::safeDirection(intendedMovementVector);
		}
	}

	Network::Player* player = Network::Players::getPlayerFromCharacter(getCharacterFromHumanoid(const_cast<Humanoid*>(this))); // this should always be safe, right?

	if (isWalking && canClickToWalk())
	{ 
		Vector3 goal = getDeltaToGoal();
		if (goal.isZero())
			intendedMovementVector = Vector3::zero();
		else
			intendedMovementVector = Math::safeDirection(getDeltaToGoal());
	}

    // this has been modified due to exploits
    float walkSpeed = getWalkSpeed();
    float percentWalkSpeed = getPercentWalkSpeed();
	answer.linear = walkSpeed * intendedMovementVector * percentWalkSpeed;
    testWalkSpeed(walkSpeed, percentWalkSpeed);

	if (autorotate)
	{
		if (player && RBX::GameBasicSettings::singleton().getRotationType() == RBX::GameBasicSettings::ROTATION_TYPE_CAMERA_RELATIVE)
		{
			answer.rotational.y = getWalkAngleError() * Humanoid::autoTurnSpeed() * 0.5f;		// hacky - multiplying angle error times auto turn speed here go get velocity
		}
		else
		{
			if (!answer.linear.isZero())
			{
				float deltaAngle = Math::radWrap(Math::getHeading(answer.linear) - getTorsoHeading());
				answer.rotational.y = autoTurnSpeed() * deltaAngle;
			}
		}
	}

	if(FLog::UserInputProfile && answer.linear.squaredLength() > 0.0001)
		FASTLOG3F(FLog::UserInputProfile, "Moving velocity: %f %f %f", answer.linear.x, answer.linear.y, answer.linear.z);

	return answer;
}

bool Humanoid::canClickToWalk() const
{
    Humanoid* thisHumanoid = const_cast<Humanoid*>(this);
    
	Network::Player* player = Network::Players::getPlayerFromCharacter( getCharacterFromHumanoid(thisHumanoid) );
	if (player)
		return clickToWalkEnabled;
	return true;
}

void Humanoid::stepWalkMode(double gameDt)
{
	if (isWalking && canClickToWalk())
	{
		if (TaskScheduler::singleton().isCyclicExecutive())
		{
			walkTimer-=gameDt;
		}
		else
		{
			walkTimer-=1.0f;
		}
		if (walkTimer <= (double) 0)
		{
			setWalkMode(false);
			moveToFinishedSignal(false);
		}
		else
		{
			float distanceToGoalSqr = getDeltaToGoal().squaredLength();
			if (distanceToGoalSqr < 1.0f)// done, close enough;
			{
				setWalkMode(false);
				moveToFinishedSignal(true);
			}
		}
	}
}

Vector3 Humanoid::getDeltaToGoal() const
{
	if (isWalking && canClickToWalk())
	{
		const PartInstance* tempTorso = getTorsoSlow();
		Vector3 worldPosition;
		if (tempTorso && hasWalkToPoint(worldPosition))
		{
			Vector3 answer = worldPosition - tempTorso->getCoordinateFrame().translation + Vector3(0,3.0f,0); // height of legs + half of torso
			answer.y = 0.0f;
			if(answer.magnitude() < 0.3f) // this is to stop minute moving of character; we are close enough
				return Vector3::zero();
			return answer;
		}
	}
	return Vector3::zero();
}

float Humanoid::getYAxisRotationalVelocity() const
{
	return (currentState.get()) 
		? currentState->getYAxisRotationalVelocity()
		: 0.0f;
}

void Humanoid::setWalkDirectionInternal(const Vector3& value, bool raiseSignal)
{
	{
		// value should be normalized
		RBXASSERT(value.x <= 1.0f && value.z <= 1.0f);

		if (fabs(value.x - walkDirection.x) > 0.000001f || fabs(value.z - walkDirection.z) > 0.000001f)
		{
			FASTLOG3F(FLog::UserInputProfile, "Setting walking direction: %f %f %f", value.x, value.y, value.z);
			if (value == Vector3::zero()) {
				walkDirection = value;		// doesn't reset the walk mode
			}
			else {
				if (value.squaredMagnitude() > 1.0f) // don't normalize again
					walkDirection = Math::safeDirection(value);
				else
					walkDirection = value;
				isWalking = false;
			}

			if (raiseSignal)
				raisePropertyChanged(propWalkDirection);
		}
	}

	// walkdirection has been set (most likely by user input)
	// cancel move direction now
	if (walkDirection != RBX::Vector3::zero())
	{
		setLuaMoveDirection(Vector3::zero());
	}
}

void Humanoid::setWalkDirection(const Vector3& value)
{
	setWalkDirectionInternal(value, true);
}

void Humanoid::setLuaMoveDirection(const Vector3& value)
{
	// might need to modify this value, so copy it
	Vector3 copyValue = value;

	if (copyValue.isZero())
	{
		copyValue = Vector3::zero();
	} 
	else if (copyValue.length() > 1.0f) 
	{
		copyValue = copyValue.unit();
	}

	FASTLOG3F(FLog::UserInputProfile, "Setting lua move direction: %f %f %f", copyValue.x, copyValue.y, copyValue.z);
	
	if (luaMoveDirection != copyValue)
	{
		luaMoveDirection = copyValue;
		if (luaMoveDirection != Vector3::zero())
		{
			setWalkMode(false); // stop WalkToPoint from moving my character now
		}
		raisePropertyChanged(propLuaMoveDirection);
        if (DFFlag::ReplicateLuaMoveDirection)
        {
            raisePropertyChanged(propLuaMoveDirectionInternal);
        }
	}
}

bool Humanoid::allow3dWalkDirection() const
{
	return ( (currentState.get()) ? currentState->getStateType() == RBX::HUMAN::SWIMMING : false);
}

void Humanoid::setWalkAngleError(const float &value)
{
	if (walkAngleError != value)
	{
		walkAngleError = value;
	}
}

void Humanoid::setWalkMode(bool walking)
{
	if (TaskScheduler::singleton().isCyclicExecutive())
	{
		walking ? walkTimer = 8.0 : walkTimer = 0;
	}
	else
	{
		walking ? walkTimer = 240 : walkTimer = 0;
	}
	isWalking = walking;
}

void Humanoid::setWalkToPoint(const Vector3& value)
{
	{
		if (walkToPoint != value) {
			walkToPoint = value;
			raisePropertyChanged(propWalkToPoint);
		}

		setWalkMode(true);
	}
	// make sure we aren't moving according to a unit vector at the same time
	setLuaMoveDirection(Vector3(0,0,0));
}

void Humanoid::setWalkToPart(PartInstance* value)
{
	if (walkToPart.get() != value) {
		walkToPart = shared_from(value);
		raisePropertyChanged(propWalkToPart);
	}

	setWalkMode(isWalking);
}

void Humanoid::setSeatPart(PartInstance* value)
{
	if (seatPart.get() != value)
	{
		seatPart = shared_from(value);
		raisePropertyChanged(propSeatPart);
	}
}

void Humanoid::setStrafe(bool value)
{
	if (strafe != value) {
		strafe = value;
		raisePropertyChanged(propStrafe);
	}
}

void Humanoid::setJumpInternal(bool value, bool replicate)
{
	if(UserInputService* userInputService = ServiceProvider::create<UserInputService>(this))
	{
		if(!userInputService->getLocalCharacterJumpEnabled())
			return;

		if (ownedByLocalPlayer)
		{
			userInputService->jumpRequestEvent();
		}
	}

	if (jump != value) {
		FASTLOG1(FLog::UserInputProfile, "Humanoid jump set: %u", value);
		jump = value;

		raisePropertyChanged(propJump);

		if (replicate)
			raisePropertyChanged(propJumpRep);
	}
}

void Humanoid::setJump(bool value)
{
	setJumpInternal(value, true);
}

void Humanoid::setAutoJump(bool value)
{	
	if(autoJump != value) 
	{
		autoJump = value;
	}
}

void Humanoid::setSit(bool value)
{
	if (sit!=value) {
		sit = value;
		raisePropertyChanged(propSit);
	}
}

void Humanoid::setAutoRotate(bool value)
{
	if (autorotate!=value) {
		autorotate = value;
		raisePropertyChanged(propAutoRotate);
	}
}

void Humanoid::setAutoJumpEnabled(bool value)
{
	if (autoJumpEnabled != value) {
		autoJumpEnabled = value;
		raisePropertyChanged(propAutoJumpEnabled);
	}
}


void Humanoid::setRagdollCriteria(int value)
{
	if (ragdollCriteria != value) {
		ragdollCriteria = value;
	}
}

void Humanoid::setPlatformStanding(bool value)
{
	if (platformStanding!=value) {
		platformStanding = value;
		raisePropertyChanged(propPlatformStanding);
	}
}
void Humanoid::setTargetPoint(const Vector3& value)
{
	float tolerance = 1.0f;		// default accuracy, studs.
	if (PartInstance* foundTorso = getTorsoSlow()) {
		float torsoToTarget = Math::taxiCabMagnitude(foundTorso->getCoordinateFrame().translation - value);
		float distance = std::max(2.0f, torsoToTarget);
		tolerance = distance / 100.0f;
	}

	if (Math::taxiCabMagnitude(value - replicatedTargetPoint) > tolerance)		// debounce target point to 1/10th of a stud
	{
		targetPoint = value;
		replicatedTargetPoint = targetPoint;
		raisePropertyChanged(propTargetPoint);
	}
}

void Humanoid::setTargetPointLocal(const Vector3& value)
{
	float tolerance = 1.0f;		// default accuracy, studs.
	if (PartInstance* foundTorso = getTorsoSlow()) {
		float torsoToTarget = Math::taxiCabMagnitude(foundTorso->getCoordinateFrame().translation - value);
		float distance = std::max(2.0f, torsoToTarget);
		tolerance = distance / 100.0f;
	}

	if (Math::taxiCabMagnitude(value - targetPoint) > tolerance)		// debounce target point to 1/10th of a stud
	{
		targetPoint = value;
	}
}

Vector3 yPlaneDirection(Vector3& v)
{
	v.y = 0.0f;
	return Math::safeDirection(v);
}

void Humanoid::moveTo2(Vector3 worldPosition, shared_ptr<Instance> part)
{
	PartInstance* p = Instance::fastDynamicCast<PartInstance>(part.get());
	moveTo(worldPosition, p);
}

void Humanoid::moveTo(const Vector3& worldPosition, PartInstance* part)
{
	// client side test here
    Vector3 local = worldPosition;
	if (part)
		local = part->getCoordinateFrame().pointToObjectSpace(worldPosition);

	walkToPoint = local + Vector3::unitX();	// will force replication
	walkToPart.reset();						// will force replication

	setWalkToPart(part);
    setWalkToPoint(local);
}

void Humanoid::move(Vector3 walkVector, bool relativeToCamera)
{
	if(DataModel* dataModel = RBX::DataModel::get(this))
	{
		if(walkVector == Vector3::zero())
		{
			setLuaMoveDirection(RBX::Vector3(0,0,0));
			rawMovementVector = RBX::Vector3(0,0,0);
			return;
		}
		else if (walkVector.length() > 1.0f)
		{
			walkVector.unitize();
		}

		// set the percent walk speed for analog style controls (like a thumbstick)
		this->setPercentWalkSpeed(walkVector.length()/1.0f);
		Vector3 movementVector = walkVector.direction();

		rawMovementVector = movementVector;

		// get the angle between the standard "forward" direction and our current desired direction
		if (relativeToCamera)
		{
			RBX::Workspace* workspace = dataModel->getWorkspace();
			if(!workspace)
				return;

			Camera* camera = workspace->getCamera();
			if(!camera)
				return;

			const RBX::Vector3 northDirection(0,0,-1);

			if (currentState.get() && currentState->getStateType() == HUMAN::SWIMMING) // we can move in three dimensions
			{
				Vector3 moveDir = camera->getCameraCoordinateFrame().vectorToWorldSpace(movementVector);
				setLuaMoveDirection(moveDir);
			}
			else // we can move in two dimensions
			{
				CoordinateFrame cframe = camera->getCameraCoordinateFrame();
				const Vector3 target = cframe.translation + (cframe.lookVector() * Vector3(1, 0, 1));
				cframe.lookAt(target, Vector3::unitY());
				Vector3 moveDir = cframe.vectorToWorldSpace(movementVector);
				moveDir.y = 0;
				if (moveDir.unitize() == 0.0f) 
				{
					moveDir = Vector3::zero();
				}
				setLuaMoveDirection(moveDir);
			}
		}
		else
		{
			setLuaMoveDirection(movementVector);
		}
	}
}

void Humanoid::setNameDisplayDistance(float d)
{
	float legalValue = G3D::max(d, 0.0f);

	if (nameDisplayDistance != legalValue)
	{
		nameDisplayDistance = legalValue;
	}
}

void Humanoid::setHealthDisplayDistance(float d)
{
	float legalValue = G3D::max(d, 0.0f);

	if (healthDisplayDistance != legalValue)
	{
		healthDisplayDistance = legalValue;
	}
}

void Humanoid::setCameraOffset(const Vector3 &value)
{
	if (value != cameraOffset)
	{
		cameraOffset = value;
	}
}


///////////////////////////////////////////////////////////

void Humanoid::setLeftLeg(PartInstance* value)
{
	// deprecated, but can be called by LUA
}

void Humanoid::setRightLeg(PartInstance* value)
{
	// deprecated, but can be called by LUA
}

void Humanoid::setTorso(PartInstance* value)
{
	// deprecated, but can be called by LUA
}
void Humanoid::setHeadMesh(DataModelMesh* value)
{
	PartInstance* foundHead;
	if ((foundHead = getHeadSlow()))
	{
		if (DataModelMesh* oldMesh = foundHead->findFirstChildOfType<DataModelMesh>()) {
			oldMesh->remove();
		}
		value->setParent(foundHead);
	}

}

 // TODOFACE
void Humanoid::setHeadDecal(Decal* val)
{
	PartInstance* foundHead;
	if ((foundHead = getHeadSlow()))
	{
		if (Decal* oldDecal = foundHead->findFirstChildOfType<Decal>())
			oldDecal->remove();
		
		val->setParent(foundHead);
	}

}

CoordinateFrame Humanoid::getTopOfHead() const
{
	//PartInstance *h = getHead();
	//RBXASSERT(h);
	//CoordinateFrame c(h->getCoordinateFrame());
	// HACK. USE EXTENTS to allow for different sized heads.
	return CoordinateFrame(G3D::Vector3(0,.5f, 0));
}

CoordinateFrame Humanoid::getRightArmGrip() const
{
	CoordinateFrame C0;

	if (getUseR15())
	{
			Attachment *answer = Tool::findFirstAttachmentByNameRecursive(getParent(), "RightGripAttachment");
			if (answer != NULL)
			{
				return answer->getFrameInPart();
			} else {
				// HACK. USE EXTENTS to allow for different sized arms.
				C0.lookAt(Vector3(0,-1,0), Vector3(0,0,-1));
				C0.translation = Vector3(0, -0.2, 0);
			}
	}
	else 
	{
		// HACK. USE EXTENTS to allow for different sized arms.
		C0.lookAt(Vector3(0,-1,0), Vector3(0,0,-1));
		C0.translation = Vector3(0, -1, 0);
	}

	return C0;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

void Humanoid::setCachePointerByType(AppendageType appendage, PartInstance *part)
{
	appendageCache[appendage] = shared_from<PartInstance>(part);
	updateBaseInstance();
}

void Humanoid::updateBaseInstance()
{
	Body* body = getRootBodyFast();
	SimBody* simBody = body ? body->getRootSimBody() : NULL;

	if (simBody && simBody->getHumanoidConnectorCount() > 0	)
		return;

	if (appendageCache[TORSO] == NULL) 
	{
		baseInstance = appendageCache[VISIBLE_TORSO];
	} 
	else 
	{
		baseInstance = appendageCache[TORSO];
	}
}

const PartInstance* Humanoid::getConstAppendageSlow(AppendageType appendage) const
{
	return appendageCache[appendage].get();
}

PartInstance* Humanoid::getAppendageSlow(AppendageType appendage)           
{
	return const_cast<PartInstance*>(getConstAppendageSlow(appendage));
}

PartInstance* Humanoid::getAppendageFast(AppendageType appendage, shared_ptr<PartInstance>& appendagePart) 
{
	PartInstance* answer = appendagePart.get();

	RBXASSERT_FISHING(		!answer 
						||	(answer == Instance::fastDynamicCast<PartInstance>(getParent()->findFirstChildByName(getAppendageString(appendage, getUseR15()))))	);

	return answer;
}

PartInstance* Humanoid::getTorsoDangerous() const		{return const_cast<PartInstance*>(getTorsoSlow());}		// reflection only
PartInstance* Humanoid::getLeftLegDangerous() const		{return const_cast<PartInstance*>(getLeftLegSlow());}	// reflection only
PartInstance* Humanoid::getRightLegDangerous() const	{return const_cast<PartInstance*>(getRightLegSlow());}	// reflection only

PartInstance* Humanoid::getTorsoSlow() 		
{
	PartInstance *retval = getAppendageSlow(TORSO);

	if (!retval)
		retval = getAppendageSlow(VISIBLE_TORSO);
	return retval;
}
PartInstance* Humanoid::getVisibleTorsoSlow() {return getAppendageSlow(VISIBLE_TORSO);}
PartInstance* Humanoid::getHeadSlow() 		{return getAppendageSlow(HEAD);}
PartInstance* Humanoid::getRightArmSlow() 	{return getAppendageSlow(RIGHT_ARM);}
PartInstance* Humanoid::getLeftArmSlow() 	{return getAppendageSlow(LEFT_ARM);}
PartInstance* Humanoid::getRightLegSlow() 	{return getAppendageSlow(RIGHT_LEG);}
PartInstance* Humanoid::getLeftLegSlow() 	{return getAppendageSlow(LEFT_LEG);}

const PartInstance* Humanoid::getTorsoSlow() const		
{
	const PartInstance *retval = getConstAppendageSlow(TORSO);

	if (!retval)
		retval = getConstAppendageSlow(VISIBLE_TORSO);
	return retval;
}
const PartInstance* Humanoid::getVisibleTorsoSlow() const		{return getConstAppendageSlow(VISIBLE_TORSO);}
const PartInstance* Humanoid::getHeadSlow() const		{return getConstAppendageSlow(HEAD);}
const PartInstance* Humanoid::getRightArmSlow() const	{return getConstAppendageSlow(RIGHT_ARM);}
const PartInstance* Humanoid::getLeftArmSlow() const	{return getConstAppendageSlow(LEFT_ARM);}
const PartInstance* Humanoid::getRightLegSlow() const	{return getConstAppendageSlow(RIGHT_LEG);}
const PartInstance* Humanoid::getLeftLegSlow() const	{return getConstAppendageSlow(LEFT_LEG);}

PartInstance* Humanoid::getTorsoFast() 		{return getAppendageFast(TORSO, appendageCache[TORSO]);}
PartInstance* Humanoid::getVisibleTorsoFast() 		{return getAppendageFast(VISIBLE_TORSO, appendageCache[VISIBLE_TORSO]);}
PartInstance* Humanoid::getHeadFast() 		{return getAppendageFast(HEAD, appendageCache[HEAD]);}
PartInstance* Humanoid::getRightArmFast() 	{return getAppendageFast(RIGHT_ARM, appendageCache[RIGHT_ARM]);}
PartInstance* Humanoid::getLeftArmFast() 	{return getAppendageFast(LEFT_ARM, appendageCache[LEFT_ARM]);}
PartInstance* Humanoid::getRightLegFast() 	{return getAppendageFast(RIGHT_LEG, appendageCache[RIGHT_LEG]);}
PartInstance* Humanoid::getLeftLegFast() 	{return getAppendageFast(LEFT_LEG, appendageCache[LEFT_LEG]);}


const StatusInstance* Humanoid::getStatusSlow() const
{
	return Instance::fastDynamicCast<StatusInstance>(findConstFirstChildByName("Status"));
}

StatusInstance* Humanoid::getStatusSlow()
{
	return Instance::fastDynamicCast<StatusInstance>(findFirstChildByName("Status"));
}

StatusInstance* Humanoid::getStatusFast()    
{
	if(!status) {
		status = shared_from(getStatusSlow());
	}
	return status.get();
}

Primitive* Humanoid::getTorsoPrimitiveSlow() {return PartInstance::getPrimitive(getTorsoSlow());}
Primitive* Humanoid::getHeadPrimitiveSlow() {return PartInstance::getPrimitive(getHeadSlow());}


void Humanoid::setName(const std::string& value)
{
	if (!ProfanityFilter::ContainsProfanity(value)){
		Instance::setName(value);	
		filterState = ContentFilter::Waiting;
	}
}

JointInstance* Humanoid::getRightShoulder()
{
	PartInstance* temp = getVisibleTorsoSlow();

	return temp ? rbx_static_cast<JointInstance*>(temp->findFirstChildByName("Right Shoulder"))
				: NULL;
}

Joint* Humanoid::getNeck()
{
	PartInstance* foundHead = getHeadSlow();
	PartInstance* foundTorso = NULL;

	foundTorso = getVisibleTorsoSlow();

	if (foundHead && foundTorso) {
		hadNeck = true;										// this humanoid once had a neck
		return Primitive::getJoint(foundHead->getPartPrimitive(), foundTorso->getPartPrimitive());
	}
	return NULL;
}

Primitive* Humanoid::getTorsoPrimitiveFast()
{
	PartInstance* answer = baseInstance.get();
	return answer ? answer->getPartPrimitive() : NULL;
}

Body* Humanoid::getRootBodyFast()
{
	Body* torsoBody = getTorsoBodyFast();
	return torsoBody ? torsoBody->getRoot() : NULL;
}


float Humanoid::getTorsoHeading() const
{
	if (const PartInstance* foundTorso = getTorsoSlow()) {
		return Math::getHeading(foundTorso->getCoordinateFrame().lookVector());
	}
	else {
		return 0.0;
	}
}

float Humanoid::getTorsoElevation() const
{
	if (const PartInstance* foundTorso = getTorsoSlow()) {
		return Math::getElevation(foundTorso->getCoordinateFrame().lookVector().direction());
	}
	else {
		return 0.0;
	}
}

// Engine Step - 1000's per second
void Humanoid::computeForce(bool throttling)
{
	if (FFlag::PhysicsSkipNonRealTimeHumanoidForceCalc)
	{
		RBXASSERT(getEngineBody() && getEngineBody()->getRootSimBody());
		if (!getEngineBody()->getRootSimBody()->isRealTimeBody())
			return;
	}

	if (world && currentState.get())			// put this here for safety
	{
		RBXASSERT(currentState.get());

		G3D::Vector3 temp;
		// The following RBXASSERT is just a way to get around the system to build this line on DEBUG build
		// The expression will always evaluate to true as we are comparing temp == temp
		// The return value of getTorsoFast()->getCoordinateFrame().translation is not bool & is Vector3.
		LEGACY_ASSERT(getTorsoFast() ? ((temp = getTorsoFast()->getCoordinateFrame().translation) == temp) : true);

		currentState->onComputeForce();

		LEGACY_ASSERT(getTorsoFast() ? ((temp - getTorsoFast()->getCoordinateFrame().translation).length() < 1.0f) : true);
	}
}

static void fireHumanoidChanged(Instance* parent)
{
    if (parent && parent->getChildren())
    {
        const Instances& children = *parent->getChildren();
        for (size_t i = 0; i < children.size(); ++i)
        {
            children[i]->humanoidChanged();
        }
    }
}

static void fireHumanoidChangedRec(Instance* parent)
{
    if (!parent)
        return;

    parent->humanoidChanged();

    if (parent->getChildren())
    {
        const Instances& children = *parent->getChildren();
        
        for (auto& child: children)
            fireHumanoidChangedRec(child.get());
    }
}

void Humanoid::updateSiblingPropertyListener(shared_ptr<PartInstance> sibling)
{
	if (sibling != NULL) 
	{
		// hook up property change notifications
		siblingMap.erase(sibling);
		if (sibling->getParent() == getParent()) 
		{
			siblingMap[sibling] = sibling->propertyChangedSignal.connect(boost::bind(&Humanoid::onEvent_SiblingPropertyChanged,this,_1));
		} 
	}
}

void Humanoid::onEvent_ChildModified(shared_ptr<Instance> child)
{
	PartInstance *pChild = Instance::fastDynamicCast<PartInstance>(child.get());

	if (pChild != NULL) 
	{
		// see if this is a humanoid part
		for (int i = TORSO; i < APPENDAGE_COUNT; i++) 
		{
			if (pChild->getName() == getAppendageString(i, getUseR15()))
			{
				if (pChild->getParent() == getParent()) 
				{
					setCachePointerByType((AppendageType) i, pChild);
				} 
				else 
				{
					setCachePointerByType((AppendageType) i, NULL);
				}
				break;
			}
		}

		updateSiblingPropertyListener(shared_from<PartInstance>(pChild));
	}
}

void Humanoid::onEvent_SiblingPropertyChanged(const RBX::Reflection::PropertyDescriptor* desc)
{
	if (desc == &Instance::desc_Name)
	{
		Instance *pParent = getParent();

		if (pParent != NULL)
		{
			// update all cache entries
			for (int i = TORSO; i < APPENDAGE_COUNT; i++)
			{
				// remove any old map for this part in case it changes due to the name change
				if (appendageCache[i] != NULL)
					siblingMap.erase(appendageCache[i]);
				appendageCache[i] = Instance::fastSharedDynamicCast<PartInstance>(shared_from(pParent->findFirstChildByName(getAppendageString(i, getUseR15()))));
				updateSiblingPropertyListener(appendageCache[i]);
			}
			updateBaseInstance();
		}
	}
}

// Into / out of the world
void Humanoid::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);		// Note - moved this to first

	if (event.child == this) 
	{
		if (event.newParent == NULL) 
		{
			// clear all cache entries
			for (int i = TORSO; i < APPENDAGE_COUNT; i++)
			{
				updateSiblingPropertyListener(appendageCache[i]);
				appendageCache[i] =  shared_ptr<PartInstance>();;
			}
			baseInstance =  shared_ptr<PartInstance>();;

			// remove old sibling change listeners
			characterChildAdded.disconnect();
			characterChildRemoved.disconnect();

		} 
		else 
		{
			// update all cache entries
			for (int i = TORSO; i < APPENDAGE_COUNT; i++)
			{
				appendageCache[i] = Instance::fastSharedDynamicCast<PartInstance>(shared_from(getParent()->findFirstChildByName(getAppendageString(i, getUseR15()))));
				updateSiblingPropertyListener(appendageCache[i]);
			}
			updateBaseInstance();

			// register update to listen to sibling changes
			characterChildAdded = getParent()->onDemandWrite()->childAddedSignal.connect(boost::bind(&Humanoid::onEvent_ChildModified, this, _1));
			characterChildRemoved = getParent()->onDemandWrite()->childRemovedSignal.connect(boost::bind(&Humanoid::onEvent_ChildModified, this, _1));
		}
	}

	World* newWorld = Workspace::getWorldIfInWorkspace(this);

	if (world != newWorld) 
	{
		if (world != NULL) {				// HACK - currentState destructor looks at world
			RBXASSERT(newWorld == NULL);

			ownedByLocalPlayer = false;
			onCFrameChangedConnection.disconnect();

			currentState.reset();	
			if (!waitingForTorso()) 
			{
				setPrimitive(0, NULL);
				setPrimitive(1, NULL);
				world->removeJoint(this);
			}
		}

		world = newWorld;

		if (world != NULL) {
			CheckTorso();
			currentState.reset(HUMAN::HumanoidState::defaultState(this));		// not balanced with the reset see below
			onLocalHumanoidEnteringWorkspace();
		}
	}

    if (DFFlag::HumanoidCookieRecursive)
    {
        fireHumanoidChangedRec(event.oldParent);
        fireHumanoidChangedRec(event.newParent);
    }
    else
    {
        fireHumanoidChanged(event.oldParent);
        fireHumanoidChanged(event.newParent);
    }
}

void Humanoid::onLocalHumanoidEnteringWorkspace()
{
	// called when Humanoid enters workspace
	Workspace* workspace = ServiceProvider::find<Workspace>(this);

	RBXASSERT(workspace);

	Humanoid *me = Humanoid::getLocalHumanoidFromContext(this);
	if (this == me)
	{
		// 1.  Update camera subject
		Camera::CameraType type = workspace->getCamera()->getCameraType();
		if (type == Camera::CUSTOM_CAMERA || 
			type == Camera::FOLLOW_CAMERA ||
			type == Camera::ATTACH_CAMERA ||
			type == Camera::TRACK_CAMERA)
		{
			if (workspace->getCamera()->getCameraSubject() != this)	workspace->getCamera()->setCameraSubject(this);
		}
		
		ownedByLocalPlayer = true;
	}
}


void Humanoid::updateLocalSimulating()
{
	PartInstance* torso = this->getTorsoSlow();
	Assembly* assembly = torso ? torso->getPartPrimitive()->getAssembly() : NULL;
	bool movingAssembly = assembly ? !assembly->computeIsGrounded() : false;

	if (!assembly) {
		localSimulating = false;
	}
	else if (movingAssembly) {
		localSimulating = !torso->computeNetworkOwnerIsSomeoneElse();
	}
}

void Humanoid::onCFrameChangedFromReflection()
{
	if (currentState.get())
		currentState->onCFrameChangedFromReflection();
}


// All humanoids client/server do this
void Humanoid::onStepped(const Stepped& event)
{
    if (!ownedByLocalPlayer && !event.longStep)
    {
        return; // if this isn't a humanoid controlled by a player, only step every other step (aka long step)
    }

	if (waitingForTorso() && !CheckTorso())
    {
		return;   // sign, still waiting for torso
    }

    RBXPROFILER_SCOPE("Physics", "Humanoid::onStepped");

	// 1. All Humanoids - update local walk mode variables
	if (event.longStep)
	{
		stepWalkMode(event.gameStep);
	}

	updateLocalSimulating();

	// 2. All Humanoids - nothing to do with local - simulate those in spatial region, otherwise "no-simulate" them
	if (localSimulating || ownedByLocalPlayer) {
		RBXASSERT(world);
		RBXASSERT(currentState.get());
		if (Instance::fastDynamicCast<ModelInstance>(getParent()))
		{
			G3D::Vector3 temp;
			// The following RBXASSERT is just a way to get around the system to build this line on DEBUG build
			// The expression will always evaluate to true as we are comapring temp == temp
			// The return value of getTorsoFast()->getCoordinateFrame().translation is not bool & is Vector3.
			RBXASSERT(getTorsoFast() ? ((temp = getTorsoFast()->getCoordinateFrame().translation) == temp) : true);

			if (!ownedByLocalPlayer && !RBX::TaskScheduler::singleton().isCyclicExecutive())
			{
				if (event.longStep)
				{
					HUMAN::HumanoidState::simulate(currentState, Constants::longUiStepDt());		// may change state
				}
			}
			else
			{
				HUMAN::HumanoidState::simulate(currentState, event.gameStep);		// may change state
			}

			RBXASSERT(getTorsoFast() ? ((temp - getTorsoFast()->getCoordinateFrame().translation).length() < 10.0f) : true);
		}

		updateHadHealth();

	}
	else
	{
		if (world)
		{
			RBXASSERT(currentState.get());
			HUMAN::HumanoidState::noSimulate(currentState);							// may change currentState
		}
	}

	if (world) 
	{
		RBXASSERT(currentState.get());
		if (currentState.get()) 
		{						
			shared_ptr<HUMAN::HumanoidState> tempState(currentState);	// Holding the current state in case of ancestor changed and a reset
			tempState->fireEvents();
		}
	}

	if(animator)
	{
		animator->onStepped(event);
	}
}


void Humanoid::renderWaypoint(Adorn* adorn, const Vector3& waypoint)
{
    if(RBX::GameBasicSettings::singleton().inClassicMode())
		DrawAdorn::cylinder(adorn, CoordinateFrame(waypoint), 1, 0.2, 1.0, Color3::green());
}

void Humanoid::render3dAdorn(Adorn* adorn)
{
	if (currentState.get())
	{
		currentState->render3dAdorn(adorn);
	}
}

void Humanoid::renderMultiplayer(Adorn* adorn, const RBX::Camera& camera)
{
	if (displayDistanceType == Humanoid::HUMANOID_DISPLAY_DISTANCE_TYPE_NONE)
		return;

	bool occludeName = false;
	float localHumanoidDistance = 0.0f;
	float localNameDisplayDistance = defaultDisplayDistance;
	float localHealthDisplayDistance = defaultDisplayDistance;
	Humanoid* localHumanoid = NULL;

	if ((localHumanoid = Humanoid::getLocalHumanoidFromContext(this)))
	{
		if(localHumanoid->getNameOcclusion() == Humanoid::NAME_OCCLUSION_ALL)
			occludeName = true;
		else if(localHumanoid->getNameOcclusion() == Humanoid::NAME_OCCLUSION_ENEMY)
			if(Teams *teams = ServiceProvider::find<Teams>(this))
				if(teams->getTeamColorForHumanoid(this) != teams->getTeamColorForHumanoid(localHumanoid))
					occludeName = true;

		if (displayDistanceType == Humanoid::HUMANOID_DISPLAY_DISTANCE_TYPE_SUBJECT)
		{
			localNameDisplayDistance = getNameDisplayDistance();
			localHealthDisplayDistance = getHealthDisplayDistance();
		} 
		else
		{
			localNameDisplayDistance = localHumanoid->getNameDisplayDistance();
			localHealthDisplayDistance = localHumanoid->getHealthDisplayDistance();
		}
	}

	if (PartInstance *foundHead = getHeadSlow()) 
	{	
		Vector3 aboveHead = foundHead->calcRenderingCoordinateFrame().translation;

		if(DFFlag::EnableMotionAnalytics)
		{
			Vector3 pos3 = aboveHead;
			Primitive * HeadPrimitive = RBX::PartInstance::getPrimitive(foundHead);
			if(HeadPrimitive)
			{
				World *HeadWorld = HeadPrimitive->getWorld();
		
				if(HeadWorld)
				{	
					double threshold = (double)(DFInt::MotionDiscontinuityThreshold) * 0.1;
					if(((pos0 - pos1).magnitude() > threshold) 
						&& ((pos1 - pos2).magnitude()  <= threshold) 
						&& ((pos2 - pos3).magnitude()  > threshold))
					{
						HeadWorld->plusErrorCount(1.0);
					}
					HeadWorld->plusPassCount(1.0);
					pos0 = pos1;
					pos1 = pos2;
					pos2 = pos3;
				}
			}
		}
		aboveHead.y += 1.5f;

		float distance = (camera.coordinateFrame().translation - aboveHead).magnitude();

		if (distance > 100.0f || foundHead->getTransparencyXml() > .99f) // if the head is 100% transparent, assume user wants this char to be invisible
			return;


		if(occludeName) // if we are occluding, do occlusion check first
			if(ContactManager* contactManager = getContactManager())
			{
				FilterHumanoidNameOcclusion filter(shared_from(this));
				if(!camera.isPartVisibleFast(*foundHead, *contactManager, &filter))
					return;
			}

		if (localHumanoid)
		{
			if (PartInstance *localHead = localHumanoid->getHeadSlow()) 
			{ 
				Vector3 localHeadPos = localHead->calcRenderingCoordinateFrame().translation;
				localHumanoidDistance = (localHeadPos - aboveHead).magnitude();
			}
		}

		float fontSize = 12.0f;
		if (distance < 20.0f)
			fontSize = 24.0f;
		else if (distance < 50.0f)
			fontSize = 18.0f;

		G3D::Color3 nameTagColor;
		Teams *teams = ServiceProvider::find<Teams>(this);
		if(teams != NULL)
			nameTagColor = teams->getTeamColorForHumanoid(this);
		else
			nameTagColor = G3D::Color3::white();


		if(this->getParent() && displayText != this->getParent()->getName())
		{
			displayText = this->getParent()->getName();
			if (!ProfanityFilter::ContainsProfanity(displayText))
				filterState = ContentFilter::Waiting;
			else
			{
				displayText = "";
				filterState = ContentFilter::Succeeded;
			}
		}

		if(filterState == ContentFilter::Waiting)
			if(ContentFilter* contentFilter = ServiceProvider::create<ContentFilter>(this))
				filterState = contentFilter->getStringState(displayText);

		const float width = (float)(fontSize * 4);

		Vector3 screenLoc = camera.project(aboveHead);
		if(screenLoc.z == std::numeric_limits<float>::infinity())
			return;

		float nameAlpha = 1.0f;
		float healthAlpha = 1.0f;
		if (localHumanoidDistance > (0.9f * localNameDisplayDistance)) {
			nameAlpha = std::max(((localNameDisplayDistance - localHumanoidDistance) / (0.1f * localNameDisplayDistance)), 0.0f);
		}
		if (localHumanoidDistance > (0.9f * localHealthDisplayDistance)) {
			healthAlpha = std::max(((localHealthDisplayDistance - localHumanoidDistance) / (0.1f * localHealthDisplayDistance)), 0.0f);
		}

		if (adorn->isVR())
		{
			return;
		}

        if (Workspace::showActiveAnimationAsset)
        {
            Instance* child = Instance::findFirstChildByName("Animator");
            if (child)
            {
                if (Animator* animator = child->fastDynamicCast<Animator>())
                {
                    std::string activeAnim = animator->getActiveAnimation();
                    adorn->drawFont2D(
                        activeAnim,
                        screenLoc.xy(),
                        fontSize,
                        false,
                        Color4(nameTagColor, nameAlpha),
                        Color4(Color3::black(), nameAlpha),
                        Text::FONT_ARIALBOLD,
                        Text::XALIGN_CENTER,
                        Text::YALIGN_BOTTOM );
                }
            }
        }
		else if(filterState == ContentFilter::Succeeded && (localNameDisplayDistance >= localHumanoidDistance))
        {
			static float alpha = 0.5f;
			Color4 strokeColor;

			static float rFactor = 0.25f;
			static float gFactor = 0.21f;
			static float bFactor = 0.0722f;
			float colorY = rFactor * nameTagColor.r + gFactor * nameTagColor.g + bFactor * nameTagColor.b;
			float nameColorL = (116.0f * std::pow(colorY, (1.0f/3.0f)) - 16.0f) / 100.0f;

			static float factor = 0.52f;
			if (nameColorL >= factor)
			{
				strokeColor = Color4(49.0f/255.0f, 49.0f/255.0f, 49.0f/255.0f, alpha * nameAlpha);
			} else {
				strokeColor = Color4(227.0f/255.0f, 227.0f/255.0f, 227.0f/255.0f, alpha * nameAlpha);
			}


			// Name
			Vector2 pos = screenLoc.xy();
			pos.y -= 2.0f;
			adorn->drawFont2D(
							displayText,
							pos,
							fontSize,
							false,
							Color4(nameTagColor, nameAlpha),
							strokeColor,
							Text::FONT_SOURCESANS,
							Text::XALIGN_CENTER,
							Text::YALIGN_BOTTOM );
        }
		// Health
		if (maxHealth != 0 && localHealthDisplayDistance >= localHumanoidDistance)
		{
			float relativeHealth = G3D::clamp(health/maxHealth, 0, 1);

			float healthHeight = 2;
			float strokeWidth = 2;
			Vector2 topLeft(screenLoc.x - 0.5f * width, screenLoc.y + 2);
			float middle = topLeft.x + (width) * relativeHealth;

			Rect healthRect(topLeft, Vector2(middle, topLeft.y + healthHeight));

			Rect backRect[3] = {
				Rect(topLeft + Vector2(0, -strokeWidth), topLeft + Vector2(width, healthHeight + strokeWidth)),
				Rect(topLeft + Vector2(-strokeWidth, -strokeWidth/2.0f), topLeft + Vector2(0, healthHeight + strokeWidth/2.0f)),
				Rect(topLeft + Vector2(width, -strokeWidth/2.0f), topLeft + Vector2(width + strokeWidth, healthHeight + strokeWidth/2.0f))
			};

			Rect strokeRect[4] = {
				Rect(topLeft + Vector2(0, -(strokeWidth*1.5f)), topLeft + Vector2(width, -strokeWidth)),
				Rect(topLeft + Vector2(0, healthHeight + strokeWidth), topLeft + Vector2(width, healthHeight + (strokeWidth*1.5f))),
				Rect(topLeft + Vector2(-(strokeWidth*1.5), -(strokeWidth*1.0f)), topLeft + Vector2(0, healthHeight + (strokeWidth*1.0f))),
				Rect(topLeft + Vector2(width, -(strokeWidth*1.0f)), topLeft + Vector2(width + strokeWidth*1.5f, healthHeight + (strokeWidth*1.0f)))
			};

			// Calc bar color
			Color4 barColor;

			static Color3 healthColors[] = { 
				Color3(255.0f/255.0f, 28.0f/255.0f, 0.0f/255.0f), 
				Color3(250.0f/255.0f, 235.0f/255.0f, 0.0f/255.0f), 
				Color3(27.0f/255.0f, 252.0f/255.0f, 107.0f/255.0f)
			};

			if (relativeHealth <= 0.1f)
			{
				barColor = Color4(healthColors[0], healthAlpha);
			} 
			else if (relativeHealth >= 0.8f)
			{
				barColor = Color4(healthColors[2], healthAlpha);
			}
			else
			{
				static float samplePoints[] = { 0.1f, 0.5f, 0.8f };
				Vector3 numeratorSum;
				float denominatorSum = 0;
				bool found = false;
				for (int i = 0; i < 3; i ++) 
				{
					float distance = fabsf(relativeHealth - samplePoints[i]);
					if (distance <= 0.0f)
					{
						barColor = Color4(healthColors[i], healthAlpha);
						found = true;
						break;
					}
					else
					{
						float wi = 1.0f / (distance * distance);
						numeratorSum = numeratorSum + wi * Vector3(healthColors[i].r, healthColors[i].g, healthColors[i].b);
						denominatorSum = denominatorSum + wi;
					}
				}
				if (!found) 
				{
					numeratorSum = numeratorSum / denominatorSum;
					barColor = Color4(numeratorSum.x, numeratorSum.y, numeratorSum.z, healthAlpha);
				}
			}

			adorn->setIgnoreTexture(true);
			Color4 backGrey(101.0f/255.0f, 100.0f/255.0f, 100.0f/255.0f, healthAlpha * 0.5);
			Color4 backGreyOpaque(100.0f/255.0f, 100.0f/255.0f, 100.0f/255.0f, healthAlpha);

			for (int i = 0; i < 4; i++)
			{ 
				adorn->rect2d(strokeRect[i].toRect2D(), Vector2::one(), Vector2::one(), barColor, strokeRect[i].toRect2D());
			}
			for (int i = 0; i < 3; i++)
			{ 
				Color4 color = backGrey;
				if (i > 0)
				{
					color = backGreyOpaque;
				}
				adorn->rect2d(backRect[i].toRect2D(), Vector2::one(), Vector2::one(), color, backRect[i].toRect2D());
			}

			adorn->rect2d(healthRect.toRect2D(), Vector2::one(), Vector2::one(), barColor, healthRect.toRect2D());
			adorn->setIgnoreTexture(false);
		}
	}
}

void Humanoid::renderBillboard(Adorn* adorn, const RBX::Camera& camera)
{
	if (displayDistanceType == Humanoid::HUMANOID_DISPLAY_DISTANCE_TYPE_NONE)
		return;

    // Get head
	PartInstance* head = getHeadSlow();
    if (!head)
        return;

	if (head->getTransparencyXml() > .99f) // if the head is 100% transparent, assume user wants this char to be invisible
		return;

    // Compute camera distance
	Vector3 aboveHead = head->calcRenderingCoordinateFrame().translation;
	aboveHead.y += 1.5f;
    
	Vector3 look = camera.getRenderingCoordinateFrame().translation - aboveHead;

	float distance = look.magnitude();

    // Compute occlusion/fade parameters
	Humanoid* localHumanoid = Humanoid::getLocalHumanoidFromContext(this);
	Teams *teams = ServiceProvider::find<Teams>(this);

	bool occludeName = false;
	float localHumanoidDistance = distance;
	float localNameDisplayDistance = defaultDisplayDistance;
	float localHealthDisplayDistance = defaultDisplayDistance;

	if (localHumanoid)
	{
		if (localHumanoid->getNameOcclusion() == Humanoid::NAME_OCCLUSION_ALL)
			occludeName = true;
		else if (localHumanoid->getNameOcclusion() == Humanoid::NAME_OCCLUSION_ENEMY)
			occludeName = (teams && teams->getTeamColorForHumanoid(this) != teams->getTeamColorForHumanoid(localHumanoid));

		if (displayDistanceType == Humanoid::HUMANOID_DISPLAY_DISTANCE_TYPE_SUBJECT)
		{
			localNameDisplayDistance = getNameDisplayDistance();
			localHealthDisplayDistance = getHealthDisplayDistance();
		} 
		else
		{
			localNameDisplayDistance = localHumanoid->getNameDisplayDistance();
			localHealthDisplayDistance = localHumanoid->getHealthDisplayDistance();
		}

		if (PartInstance* localHead = localHumanoid->getHeadSlow())
		{ 
			Vector3 localHeadPos = localHead->calcRenderingCoordinateFrame().translation;

			localHumanoidDistance = (localHeadPos - aboveHead).magnitude();
		}
	}
    
    // Fast early-out
   	if (localHumanoidDistance >= localNameDisplayDistance && localHumanoidDistance >= localHealthDisplayDistance)
        return;

    // Occlude if necessary
	if (occludeName)
    {
		if (ContactManager* contactManager = getContactManager())
		{
			FilterHumanoidNameOcclusion filter(shared_from(this));
			if (!camera.isPartVisibleFast(*head, *contactManager, &filter))
				return;
		}
    }

    // Update display text
	if (displayText != getParent()->getName())
	{
		displayText = getParent()->getName();

		if (ProfanityFilter::ContainsProfanity(displayText))
			displayText = "";
	}

    // Compute name tag color
	Color3 nameTagColor = teams ? teams->getTeamColorForHumanoid(this) : Color3::white();

    // Compute name/health fade
	float nameAlpha = 1.0f;
	float healthAlpha = 1.0f;
    
	if (localHumanoidDistance > (0.9f * localNameDisplayDistance))
		nameAlpha = std::max(((localNameDisplayDistance - localHumanoidDistance) / (0.1f * localNameDisplayDistance)), 0.0f);

	if (localHumanoidDistance > (0.9f * localHealthDisplayDistance))
		healthAlpha = std::max(((localHealthDisplayDistance - localHumanoidDistance) / (0.1f * localHealthDisplayDistance)), 0.0f);
    
    // Render!
    if (adorn->isVR())
    {
        // Compute font size
        float fontSize = 18.0f;
    
        CoordinateFrame cframe(aboveHead);
        
        Vector3 heading = Vector3(look.x, 0, look.z);
        
        cframe.lookAt(aboveHead - heading);
        
        // Keep constant world space size, with the height of text being ~1 stud
        cframe.rotation *= 1 / fontSize;
        
        AdornSurface adornView(adorn, Rect2D(), cframe);
        renderBillboardImpl(&adornView, Vector2(0, 0), fontSize, nameTagColor, nameAlpha, healthAlpha);
    }
    else
    {
        // Project to screen
        Vector3 screenLoc = camera.project(aboveHead);
        if (screenLoc.z == std::numeric_limits<float>::infinity())
            return;

        // Compute font size
        float fontSize = 12.0f;
        if (distance < 20.0f)
            fontSize = 24.0f;
        else if (distance < 50.0f)
            fontSize = 18.0f;
    
        renderBillboardImpl(adorn, screenLoc.xy(), fontSize, nameTagColor, nameAlpha, healthAlpha);
    }
}

void Humanoid::renderBillboardImpl(Adorn* adorn, const Vector2& screenLoc, float fontSize, const Color3& nameTagColor, float nameAlpha, float healthAlpha)
{
	float width = fontSize * 4;

	if (nameAlpha > 0)
    {
		static float alpha = 0.5f;
		Color4 strokeColor;

		static float rFactor = 0.25f;
		static float gFactor = 0.21f;
		static float bFactor = 0.0722f;
		float colorY = rFactor * nameTagColor.r + gFactor * nameTagColor.g + bFactor * nameTagColor.b;
		float nameColorL = (116.0f * std::pow(colorY, (1.0f/3.0f)) - 16.0f) / 100.0f;

		static float factor = 0.52f;

		if (nameColorL >= factor)
			strokeColor = Color4(49.0f/255.0f, 49.0f/255.0f, 49.0f/255.0f, alpha * nameAlpha);
		else
			strokeColor = Color4(227.0f/255.0f, 227.0f/255.0f, 227.0f/255.0f, alpha * nameAlpha);

		// Name
		Vector2 pos = screenLoc;
		pos.y -= 2.0f;
		adorn->drawFont2D(displayText, pos, fontSize, false, Color4(nameTagColor, nameAlpha), strokeColor, Text::FONT_SOURCESANS, Text::XALIGN_CENTER, Text::YALIGN_BOTTOM);
    }

	// Health
	if (maxHealth != 0 && healthAlpha > 0)
	{
		float relativeHealth = G3D::clamp(health/maxHealth, 0, 1);

		float healthHeight = 2;
		float strokeWidth = 2;
		Vector2 topLeft(screenLoc.x - 0.5f * width, screenLoc.y + 2);
		float middle = topLeft.x + (width) * relativeHealth;

		Rect healthRect(topLeft, Vector2(middle, topLeft.y + healthHeight));

		Rect backRect[3] =
        {
			Rect(topLeft + Vector2(0, -strokeWidth), topLeft + Vector2(width, healthHeight + strokeWidth)),
			Rect(topLeft + Vector2(-strokeWidth, -strokeWidth/2.0f), topLeft + Vector2(0, healthHeight + strokeWidth/2.0f)),
			Rect(topLeft + Vector2(width, -strokeWidth/2.0f), topLeft + Vector2(width + strokeWidth, healthHeight + strokeWidth/2.0f))
		};

		Rect strokeRect[4] =
        {
			Rect(topLeft + Vector2(0, -(strokeWidth*1.5f)), topLeft + Vector2(width, -strokeWidth)),
			Rect(topLeft + Vector2(0, healthHeight + strokeWidth), topLeft + Vector2(width, healthHeight + (strokeWidth*1.5f))),
			Rect(topLeft + Vector2(-(strokeWidth*1.5), -(strokeWidth*1.0f)), topLeft + Vector2(0, healthHeight + (strokeWidth*1.0f))),
			Rect(topLeft + Vector2(width, -(strokeWidth*1.0f)), topLeft + Vector2(width + strokeWidth*1.5f, healthHeight + (strokeWidth*1.0f)))
		};

		// Calc bar color
		static const Color3 healthColors[] =
        {
			Color3(255.0f/255.0f, 28.0f/255.0f, 0.0f/255.0f), 
			Color3(250.0f/255.0f, 235.0f/255.0f, 0.0f/255.0f), 
			Color3(27.0f/255.0f, 252.0f/255.0f, 107.0f/255.0f)
		};

		Color4 barColor;

		if (relativeHealth <= 0.1f)
		{
			barColor = Color4(healthColors[0], healthAlpha);
		} 
		else if (relativeHealth >= 0.8f)
		{
			barColor = Color4(healthColors[2], healthAlpha);
		}
		else
		{
			static float samplePoints[] = { 0.1f, 0.5f, 0.8f };
			Vector3 numeratorSum;
			float denominatorSum = 0;
			bool found = false;
			for (int i = 0; i < 3; i ++) 
			{
				float distance = fabsf(relativeHealth - samplePoints[i]);
				if (distance <= 0.0f)
				{
					barColor = Color4(healthColors[i], healthAlpha);
					found = true;
					break;
				}
				else
				{
					float wi = 1.0f / (distance * distance);
					numeratorSum = numeratorSum + wi * Vector3(healthColors[i].r, healthColors[i].g, healthColors[i].b);
					denominatorSum = denominatorSum + wi;
				}
			}
			if (!found) 
			{
				numeratorSum = numeratorSum / denominatorSum;
				barColor = Color4(numeratorSum.x, numeratorSum.y, numeratorSum.z, healthAlpha);
			}
		}

		adorn->setIgnoreTexture(true);

		Color4 backGrey(101.0f/255.0f, 100.0f/255.0f, 100.0f/255.0f, healthAlpha * 0.5);
		Color4 backGreyOpaque(100.0f/255.0f, 100.0f/255.0f, 100.0f/255.0f, healthAlpha);

		for (int i = 0; i < 4; i++)
		{ 
			adorn->rect2d(strokeRect[i].toRect2D(), Vector2::one(), Vector2::one(), barColor, strokeRect[i].toRect2D());
		}

		for (int i = 0; i < 3; i++)
		{ 
			Color4 color = (i == 0) ? backGrey : backGreyOpaque;

			adorn->rect2d(backRect[i].toRect2D(), Vector2::one(), Vector2::one(), color, backRect[i].toRect2D());
		}

		adorn->rect2d(healthRect.toRect2D(), Vector2::one(), Vector2::one(), barColor, healthRect.toRect2D());
		adorn->setIgnoreTexture(false);
	}
}

Vector3 Humanoid::render3dSortedPosition() const
{
	if (const PartInstance *foundHead = getHeadSlow()) 
	{
		return foundHead->getTranslationUi();
	}
	return Vector3();
}

void Humanoid::render3dSortedAdorn(Adorn* adorn)
{
    if (!ownedByLocalPlayer)
    {
        if (FFlag::HumanoidRenderBillboard)
            renderBillboard(adorn, *adorn->getCamera());
		else if (FFlag::HumanoidRenderBillboardVR && adorn->isVR())
            renderBillboard(adorn, *adorn->getCamera());
        else
            renderMultiplayer(adorn, *adorn->getCamera());
    }
}

static void collectAllDescendantCharacterPrims(shared_ptr<RBX::Instance> descendant, std::vector<Primitive*>& primitives)
{
	PartInstance *part = Instance::fastDynamicCast<PartInstance>(descendant.get());
	if (part) 
	{
		primitives.push_back(part->getPartPrimitive());
	}
}

void Humanoid::getPrimitives(std::vector<Primitive*>& primitives) const
{
	// Get prims of all parts in character model
	const ModelInstance *m = getConstCharacterFromHumanoid(this);
	if (!m) return;

	m->visitDescendants(boost::bind(&collectAllDescendantCharacterPrims, _1, boost::ref(primitives)));
}

static void collectAllDescendantCharacterParts(shared_ptr<RBX::Instance> descendant, std::vector<PartInstance*>& parts)
{
	if (PartInstance *part = Instance::fastDynamicCast<PartInstance>(descendant.get())) 
	{
		parts.push_back(part);
	}
}

void Humanoid::getParts(std::vector<PartInstance*>& parts) const
{
	// Get all parts in character model
	const ModelInstance *m = getConstCharacterFromHumanoid(this);
	if (!m) return;

	m->visitDescendants(boost::bind(&collectAllDescendantCharacterParts, _1, boost::ref(parts)));
}

const CoordinateFrame Humanoid::getRenderLocation()
{
	PartInstance* foundHead = NULL;

	if (getDead())
	{
		foundHead = getVisibleTorsoSlow();
	} else {
		foundHead = getTorsoSlow();
	}

	if (foundHead) {
		CoordinateFrame frame = foundHead->calcRenderingCoordinateFrame();
		frame = frame * headTorsoOffset; // apply offset from root CFrame to approximate head location
		frame = frame * cameraOffset;
		return frame;
	}
	else
		return CoordinateFrame();
}

const Vector3 Humanoid::getRenderSize()
{
	if(ModelInstance* character = getCharacterFromHumanoid(this))
		return character->calculateModelSize();
	else
		return Vector3(0,0,0);
}

const CoordinateFrame Humanoid::getLocation() 
{
	if (PartInstance* foundHead = getHeadSlow()) {
		return foundHead->getCoordinateFrame();
	}
	else {
		return CoordinateFrame();
	}
}

void Humanoid::getSelectionIgnorePrimitives(std::vector<const Primitive*>& primitives)
{
	// TODO: This cast blows. We should refactor getPrimitives to use back_insert_iterator or a for_each pattern...
	std::vector<Primitive*>& temp = reinterpret_cast<std::vector<Primitive*>&>(primitives);

	getPrimitives(temp);
}

void Humanoid::getCameraIgnorePrimitives(std::vector<const Primitive*>& primitives)
{
	getSelectionIgnorePrimitives(primitives);

	FilteredSelection<PartInstance>* sel = ServiceProvider::create< FilteredSelection<PartInstance> >(this);
	if (sel) {
		for(std::vector<PartInstance *>::const_iterator iter = sel->begin(); iter != sel->end(); iter++) {
			primitives.push_back((*iter)->getPartPrimitive());
		}
	}
}


static void setLocalTransparencyModifierDecendants(shared_ptr<RBX::Instance> descendant, float transparencyModifier)
{
	PartInstance *part = Instance::fastDynamicCast<PartInstance>(descendant.get());
	if (part) 
	{
		if (Instance::fastDynamicCast<Tool>(part->getParent()) == NULL)
			part->setLocalTransparencyModifier(transparencyModifier); // don't ever make tools transparent.
	} 
	else if (Decal* d = Instance::fastDynamicCast<Decal>(descendant.get()))
	{
		d->setTransparency(transparencyModifier);
	}
}

void Humanoid::setLocalTransparencyModifier(float transparencyModifier) const
{
	// Get prims of all parts in character model
	const ModelInstance *m = getConstCharacterFromHumanoid(this);
	if (!m) return;

	m->visitDescendants(boost::bind(&setLocalTransparencyModifierDecendants, _1, transparencyModifier));
}

void Humanoid::tellCameraNear(float distance) const
{
	float localTransparencyModifier = 0.0f;
	if (isDistanceFirstPerson(distance) && !isTransitioning())
		localTransparencyModifier = 1.0f;
	else if (distance < (1.5f * firstPersonCutoff()))
		localTransparencyModifier = std::min(1.0f - (distance - (1.0f * firstPersonCutoff())) /  (0.5f * firstPersonCutoff()), 1.0f);

	setLocalTransparencyModifier(localTransparencyModifier);
}

void Humanoid::tellCameraSubjectDidChange(shared_ptr<Instance> oldSubject, shared_ptr<Instance> newSubject) const
{
	if( (this == oldSubject.get()) && (this != newSubject.get()) )
		setLocalTransparencyModifier(0.0f); // insures humanoid's character is not invisible on camera subject change
}

void Humanoid::tellCursorOver(float cursorOffset) const
{
	std::vector<Primitive*> primitives;
	getPrimitives(primitives);

	float localTransparencyModifier = 0.0f;
	if (isFirstPerson() && !isTransitioning())
		localTransparencyModifier = 1.0f;
	else if (cursorOffset < 0.5f)
		localTransparencyModifier = (0.5f - cursorOffset);

	for (size_t i = 0; i < primitives.size(); ++i) {
		PartInstance* part = PartInstance::fromPrimitive(primitives[i]);
		if (Instance::fastDynamicCast<Tool>(part->getParent()) == NULL)
		{
			part->setLocalTransparencyModifier(localTransparencyModifier); // don't ever make tools transparent.
		}
	}
}

void Humanoid::setFirstPersonRotationalVelocity(const Vector3& desiredLook, bool firstPersonOn)
{
	if (!firstPersonOn || !autorotate || (DFFlag::FixedSitFirstPersonMove && sit))
	{
        // FYI: when sitting in not-anchored Seat objects, this check is not necessary since the rotation is reverted back later this heartbeat
		setWalkAngleError(0.0f);
		return;
	}
	else
	{
		if (PartInstance* foundTorso = this->getTorsoSlow())
		{
			Vector3 adjustedLook = desiredLook;
			UserInputService* uis = ServiceProvider::find<UserInputService>(this);
			if (uis && DFFlag::RotateFirstPersonInVR && uis->getVREnabled())
			{
				// Rotate desiredLook by the head yaw rotation
				Matrix3 headRotation = uis->getUserHeadCFrame().rotation;
				adjustedLook = headRotation * desiredLook;
			}

			Vector3 look = foundTorso->getCoordinateFrame().lookVector();
			float lookAng = Math::getHeading(look);
			float desiredLookAng = Math::getHeading(adjustedLook);

			float dl = Math::radWrap(desiredLookAng - lookAng);

			if (currentState.get() &&
				currentState->getStateType() != HUMAN::SWIMMING)
			{
				CoordinateFrame currentPosition = foundTorso->getCoordinateFrame();
				Math::rotateAboutYGlobal(currentPosition, dl);
				if (autorotate) {
					foundTorso->setPhysics(currentPosition);
				}
			}

			if (fabs(dl) < .1f) dl = 0.0f; // 3 degrees of slosh, so we don't spam small updates over the network.

			setWalkAngleError(dl);	// warning - big hack - setting walk rotational velocity to the angle error!
		}
	}
}

namespace Reflection {
template<>
EnumDesc<RBX::Humanoid::NameOcclusion>::EnumDesc()
:EnumDescriptor("NameOcclusion")
{
	addPair(RBX::Humanoid::NAME_OCCLUSION_ALL, "OccludeAll");
	addPair(RBX::Humanoid::NAME_OCCLUSION_ENEMY, "EnemyOcclusion");
	addPair(RBX::Humanoid::NAME_OCCLUSION_NONE, "NoOcclusion");
}

template<>
EnumDesc<RBX::Humanoid::HumanoidDisplayDistanceType>::EnumDesc()
:EnumDescriptor("HumanoidDisplayDistanceType")
{
	addPair(RBX::Humanoid::HUMANOID_DISPLAY_DISTANCE_TYPE_VIEWER, "Viewer");
	addPair(RBX::Humanoid::HUMANOID_DISPLAY_DISTANCE_TYPE_SUBJECT, "Subject");
	addPair(RBX::Humanoid::HUMANOID_DISPLAY_DISTANCE_TYPE_NONE, "None");
}

template<>
EnumDesc<RBX::Humanoid::HumanoidRigType>::EnumDesc()
	:EnumDescriptor("HumanoidRigType")
{
	addPair(RBX::Humanoid::HUMANOID_RIG_TYPE_R6, "R6");
	addPair(RBX::Humanoid::HUMANOID_RIG_TYPE_R15, "R15");
}


template<>
RBX::Humanoid::NameOcclusion& Variant::convert<RBX::Humanoid::NameOcclusion>(void)
{
	return genericConvert<RBX::Humanoid::NameOcclusion>();
}

template<>
EnumDesc<RBX::Humanoid::Status>::EnumDesc()
:EnumDescriptor("Status")
{
	addPair(RBX::Humanoid::POISON_STATUS, "Poison");
	addPair(RBX::Humanoid::CONFUSION_STATUS, "Confusion");
}

template<>
RBX::Humanoid::Status& Variant::convert<RBX::Humanoid::Status>(void)
{
	return genericConvert<RBX::Humanoid::Status>();
}
}  // namespace Reflection

template<>
bool RBX::StringConverter<RBX::Humanoid::NameOcclusion>::convertToValue(const std::string& text, RBX::Humanoid::NameOcclusion& value)
{
	if(text.find("OccludeAll")){
		value = RBX::Humanoid::NAME_OCCLUSION_ALL;
		return true;
	}
	if(text.find("EnemyOcclusion")){
		value = RBX::Humanoid::NAME_OCCLUSION_ENEMY;
		return true;
	}
	if(text.find("NoOcclusion")){
		value = RBX::Humanoid::NAME_OCCLUSION_NONE;
		return true;
	}
	return false;
}

template<>
bool RBX::StringConverter<RBX::Humanoid::Status>::convertToValue(const std::string& text, RBX::Humanoid::Status& value)
{
	if(text.find("Poison")){
		value = RBX::Humanoid::POISON_STATUS;
		return true;
	}
	if(text.find("Confusion")){
		value = RBX::Humanoid::CONFUSION_STATUS;
		return true;
	}
	return false;
}

void Humanoid::setupAnimator()
{
	if(!animator)
	{
		animator = shared_from<Animator>(findFirstChildOfType<Animator>());

		if (!animator)
		{
			animator = shared_ptr<Animator>(Creatable<Instance>::create<RBX::Animator>(this));
			if (getParent()->getClassNameStr() == "Model")
				animator->setParent(this);
		}
	}
}

Animator* Humanoid::getAnimator()
{
	setupAnimator();
	RBXASSERT(animator);
	return animator.get();
}

shared_ptr<const Reflection::ValueArray> Humanoid::getPlayingAnimationTracks()
{
	return getAnimator()->getPlayingAnimationTracks();
}

shared_ptr<Instance> Humanoid::loadAnimation(shared_ptr<Instance> instance)
{
	return getAnimator()->loadAnimation(instance);
}

bool Humanoid::getStateTransitionEnabled(HUMAN::StateType state)
{
    if (state < HUMAN::FALLING_DWN || state >= HUMAN::NUM_STATE_TYPES)
    {
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Invalid state passed to GetStateEnabled.");
    } 
    else 
    {        
        return stateTransitionEnabled[state];
    }
    return true;
}

void Humanoid::setStateTransitionEnabled(HUMAN::StateType state, bool enabled)
{
    if (state < HUMAN::FALLING_DWN || state >= HUMAN::NUM_STATE_TYPES)
    {
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Invalid state passed to SetStateEnabled.");
    } 
    else 
    {        
        stateTransitionEnabled[state] = enabled;
        stateEnabledChangedSignal(state, enabled);
    }
}

bool Humanoid::computeNearlyTouched()
{
	// This will not detect close-by objects accelerated from zero to high speed
	PartInstance* part = getVisibleTorsoSlow();
	Primitive* prim = part ? part->getPartPrimitive() : NULL;
	if ( prim )
	{
		int nearCount = 0;
		for ( int i = 0; i < prim->getNumContacts(); ++i)
		{
			Contact* contact = prim->getContact(i);
			RBXASSERT( contact );
			if ( !contact->isInContact() )
				++nearCount;
		}					
		if ( nearCount > numContacts )
		{
			numContacts = nearCount;
			return true;
		}
		numContacts = nearCount;
	}
	return false;
}

bool Humanoid::isLegalForClientToChange( const Reflection::PropertyDescriptor& desc ) const
{
	if (desc == propJump)
		return true;
	if (desc == propWalkDirection)
		return true;
	if (desc == propWalkToPoint)
		return true;
	if (desc == propWalkToPart)
		return true;
	if (desc == propLuaMoveDirection)
		return true;
	return false;
}

bool Humanoid::CheckTorso()
{
	Primitive* torso = getTorsoPrimitiveSlow();
	if (!torso || !world)
		return false;
	setPrimitive(0, torso);
	setPrimitive(1, world->getGroundPrimitive());
	world->insertJoint(this);
	onCFrameChangedConnection = PartInstance::fromPrimitive(torso)->onDemandWrite()->cframeChangedFromReflectionSignal.connect(boost::bind(&Humanoid::onCFrameChangedFromReflection, this));
	torsoArrived = true;
	return true;
}

int Humanoid::getPlayerId()
{
	if (Network::Player* player = Network::Players::getPlayerFromCharacter(getParent()))
	{
		return player->getUserID();
	}

	return INT_MAX;
}

Vector3 findClosestTorsoCornerToTeleportGoal(const Vector3& torsoSize, const Vector3& torsoLocalSpaceGoal)
{
	// Look through 4 corners of the Humanoid Torso in the XZ Plane and find the furthest corner.
	// If only we had Shape Casts
	Vector3 closestCorner(0,0,0);

	for (size_t i = 0; i < 4; i++)
	{
		int xSign = 1;
		int zSign = 1;
		if (i > 2)
			xSign = -1;
		if (i % 2)
			zSign = -1;

		Vector3 currentCorner(xSign*torsoSize.x/2, 0 , zSign*torsoSize.z/2);
				
		if (currentCorner.dot(torsoLocalSpaceGoal) > closestCorner.dot(torsoLocalSpaceGoal))
		{
			closestCorner = currentCorner;
		}
	}
	
	return closestCorner;
}

bool foundShortestDistanceFromCornerToObject(PartInstance* floorPart, Humanoid* charHumanoid, const Vector3& deltaDisplacementGoal, const Vector3& torsoSpaceClosestCorner, Vector3& allowedDisplacementOut)
{
	bool foundShorterDelta = false;
	World* characterWorld = charHumanoid->getWorld();

	if ( characterWorld == NULL )
		return false;

	RBX::ContactManager* contactManager = characterWorld->getContactManager();
	CoordinateFrame torsoCFrame = charHumanoid->getTorsoPrimitiveFast()->getCoordinateFrame();
	allowedDisplacementOut = deltaDisplacementGoal;

	for (size_t i = 0; i < 3; i++)
	{
		int xSign = 1;
		int zSign = 1;

		// Find adjacent corners by multiplying local X and Z coordinates by -1 in alternating order
		if (i == 1)
			zSign = -1;
		if (i == 2)
			xSign = -1;
		
		Vector3 closestCornerAdjacents(xSign*torsoSpaceClosestCorner.x, 0 , zSign*torsoSpaceClosestCorner.z);
		Vector3 worldSpaceCurrentCorner = torsoCFrame.pointToWorldSpace(closestCornerAdjacents);


		// Raycast Setup
		RbxRay newRay(worldSpaceCurrentCorner, deltaDisplacementGoal);
		std::vector<const Primitive*> dummy;
		Vector3 hitLocationOut;
		FilterSameAssembly filterHumanoids(shared_from(charHumanoid->getTorsoFast()));
		FilterSameAssembly filterFloor(shared_from(floorPart));
		MergedFilter combinedFilter(&filterHumanoids, &filterFloor);
		FilterHumanoidParts humanoidFilter;

		// Raycast
		Primitive* hitPrim = contactManager->getHit(newRay,
														&dummy,
														&combinedFilter,
														hitLocationOut);

		if (hitPrim)
		{
			Vector3 foundDisplacementAllowed = hitLocationOut - worldSpaceCurrentCorner;

			// if foundDisplacementAllowed is shorter than last allowedDisplacementOut, we found an obstacle even earlier in the way.
			if ((foundDisplacementAllowed).squaredMagnitude() < allowedDisplacementOut.squaredMagnitude())
			{
				foundShorterDelta = true;
				allowedDisplacementOut = foundDisplacementAllowed;
			}
		}
	}

	return foundShorterDelta;
}

void Humanoid::truncateDisplacementIfObstacle(PartInstance* floorPart, Vector3& newCharPosition, const Vector3& previousCharPosition)
{
	Primitive* torsoPrim = this->getTorsoPrimitiveFast();
	if (torsoPrim)
	{
		// When the Player does not simulate the floor, the floor teleports around based on Network updates
		// This may cause the player to teleport around obstacles, so we predict if there are obstacles in the path and limit the teleportation based
		// on obstacles
		Vector3 deltaDisplacement = newCharPosition - previousCharPosition;
		if (deltaDisplacement.squaredMagnitude() > 0.0f)
		{
			CoordinateFrame torsoCFrame = torsoPrim->getCoordinateFrame();
			PartInstance* torso = PartInstance::fromPrimitive(torsoPrim);

			// Find appropriate origin of Raycast
			Vector3 closestCorner = findClosestTorsoCornerToTeleportGoal(torso->getPartSizeUi(), torsoCFrame.pointToObjectSpace(newCharPosition));

			// Find Closest Obstacle To Goal
			Vector3 maxDisplacementAllowed;
			if (foundShortestDistanceFromCornerToObject(floorPart, this, deltaDisplacement, closestCorner, maxDisplacementAllowed))
			{
				newCharPosition = previousCharPosition + deltaDisplacement.unit() * sqrt((maxDisplacementAllowed.dot(maxDisplacementAllowed)));
				FASTLOG3F(FLog::HumanoidFloorProcess, "Obstacle Found, Moving to: %4.4f, %4.4f, %4.4f", newCharPosition.x, newCharPosition.y, newCharPosition.z);
			}
		}
	}
}

Vector3 Humanoid::getSimulatedFrictionVelocityOffset(PartInstance* floorPart, Vector3& newCharPosition, const Vector3& previousCharPosition, const Vector3& charPosInFloorSpace, const RBX::Velocity& previousFloorVelocity, float& netDt)
{
	Primitive* floorPrim = floorPart->getPartPrimitive();
	if (floorPrim && (getCurrentFloorFilterPhase() >= Assembly::NoSim_SendIfSim) && floorPrim->getWorld())
	{
        Assembly* floorAssy = floorPrim->getAssembly();
        Primitive* floorRootPrim = NULL;
		if (floorAssy)
		{
            floorRootPrim = floorAssy->getAssemblyPrimitive();
			if (floorRootPrim == NULL)
			{
				return Vector3(0,0,0);
			}
		}


		Vector3 newLinearVelocityAtGoal = (floorRootPrim->getBody()->getVelocity().linearVelocityAtOffset(charPosInFloorSpace));
		Vector3 oldLinearVelocityOnFloor = previousFloorVelocity.linearVelocityAtOffset(charPosInFloorSpace);
		Vector3 deltaVelocity(newLinearVelocityAtGoal - oldLinearVelocityOnFloor);

		float multiplyer = 0.0f;
		if (getWorld()->getUsingPGSSolver())
			multiplyer = RBX::HUMAN::HumanoidState::runningKMovePForPGS() * netDt * (1000.0f/(DFInt::HumanoidFloorManualFrictionVelocityMultValue));
		else
			multiplyer = RBX::HUMAN::HumanoidState::runningKMoveP() * netDt * (1000.0f/(DFInt::HumanoidFloorManualFrictionVelocityMultValue));

		Vector3 approxXZForce(deltaVelocity.x * multiplyer , 0 , deltaVelocity.z * multiplyer);
		float forceMagnitudeOverMax = (approxXZForce.magnitude() / (RBX::HUMAN::HumanoidState::maxLinearGroundMoveForce()));  
											

		// If the approximate force application by this "Network Update" is larger than 
		// Max XZ Force allowed, then we have to modify our position update.
		Vector3 deltaDisplacement = newCharPosition - previousCharPosition;
		if (forceMagnitudeOverMax > 1.0f && deltaDisplacement.squaredMagnitude() > 0.0f)
		{
			Vector3 velocityOffset;
			// If acceleration is in Displacement Dir
			Vector3 deltaVelocity = newLinearVelocityAtGoal - oldLinearVelocityOnFloor;
			if ((deltaVelocity).dot(deltaDisplacement) >= 0.0f)
			{
				velocityOffset = -(newCharPosition - previousCharPosition).unit() * (deltaVelocity - deltaVelocity*(1/forceMagnitudeOverMax)).magnitude();
			}
			else
			{
				velocityOffset = (newCharPosition - previousCharPosition).unit() * (deltaVelocity - deltaVelocity*(1/forceMagnitudeOverMax)).magnitude();
			}
			FASTLOG1F(FLog::HumanoidFloorProcess, "forceMagnitudeOverMax: %4.4f", forceMagnitudeOverMax);
			FASTLOG3F(FLog::HumanoidFloorProcess, "Accelerating too fast, Applying Velocity: %4.4f, %4.4f, %4.4f", velocityOffset.x, velocityOffset.y, velocityOffset.z);
			return velocityOffset;
		}	
	}
	return Vector3(0,0,0);
}

void Humanoid::updateNetworkFloorPosition(PartInstance* floorPart, CoordinateFrame& previousFloorPosition, RBX::Velocity& lastFloorVelocity, float& netDt)
{
	Body* rootHumanBody = getRootBodyFast();
	if (floorPart && getWorld() && rootHumanBody && validateNetworkUpdateDistance(floorPart, previousFloorPosition, netDt))
	{
		Primitive* floorPrimitive = floorPart->getPartPrimitive();
		if (floorPrimitive->getWorld() == NULL)
			return;
		
		Vector3 charPosInObjectSpace = previousFloorPosition.pointToObjectSpace(rootHumanBody->getCoordinateFrame().translation);
		Vector3 charPosInWorldSpaceGoal =  floorPrimitive->getCoordinateFrame().pointToWorldSpace(charPosInObjectSpace);
		Matrix3 deltaRotation = (floorPrimitive->getCoordinateFrame().rotation * previousFloorPosition.rotation.transpose());
		// No Update needed if no rotation or displacement needs to be applied.
		// TODO: Fix this part for optimization. For some reason this causes many false
		// early outs when in BufferZone.
		if ((floorPrimitive->getCoordinateFrame().translation - previousFloorPosition.translation).squaredMagnitude() < epsilonDisplacementSqr  
			&& (floorPrimitive->getBody()->getVelocity() - lastFloorVelocity).linear.squaredMagnitude() < epsilonDisplacementSqr)
		{
			Vector3 eulerDeltaRot; 
			deltaRotation.toEulerAnglesXYZ(eulerDeltaRot.x, eulerDeltaRot.y, eulerDeltaRot.z);
			
			if ((floorPrimitive->getBody()->getVelocity() - lastFloorVelocity).rotational.squaredMagnitude() < epsilonDisplacementSqr && eulerDeltaRot.squaredMagnitude() < epsilonDisplacementSqr)
			{
				FASTLOG1F(FLog::HumanoidFloorProcess, "Floor movement too small: %4.4f",  (floorPrimitive->getCoordinateFrame().translation - previousFloorPosition.translation).squaredMagnitude());
				return;
			}
		}


		// Check for Obstacles
		truncateDisplacementIfObstacle(floorPart, charPosInWorldSpaceGoal, rootHumanBody->getCoordinateFrame().translation);

		
		// Grab Y Rotation ONLY
		Vector3 eulAngleRotation(0,0,0);
		deltaRotation.toEulerAnglesYZX(eulAngleRotation.y, eulAngleRotation.z, eulAngleRotation.x);
		Matrix3 deltaRotationAboutY = G3D::Matrix3::fromEulerAnglesYZX(eulAngleRotation.y, 0, 0);

		// Create new Coordinate Frame
		CoordinateFrame newFrame(rootHumanBody->getCoordinateFrame().rotation * deltaRotationAboutY);
		newFrame = newFrame + charPosInWorldSpaceGoal;
		
		Vector3 deltaDisplacement = (floorPrimitive->getCoordinateFrame().translation - previousFloorPosition.translation);
		FASTLOG3F(FLog::HumanoidFloorProcess,  "Pre Friction Displacement: %4.4f, %4.4f, %4.4f", deltaDisplacement.x, deltaDisplacement.y, deltaDisplacement.z);
		Vector3 addedVelocity = getSimulatedFrictionVelocityOffset(floorPart, charPosInWorldSpaceGoal,  rootHumanBody->getCoordinateFrame().translation, charPosInObjectSpace, lastFloorVelocity, netDt);
		
		getRootBodyFast()->setPv(PV(newFrame, rootHumanBody->getVelocity() + addedVelocity), *getTorsoPrimitiveFast());
		Vector3 newCharPosInObjectSpace = floorPrimitive->getCoordinateFrame().pointToObjectSpace(rootHumanBody->getCoordinateFrame().translation);
		FASTLOG3F(FLog::HumanoidFloorProcess,  "Platform displaced: %4.4f, %4.4f, %4.4f", deltaDisplacement.x, deltaDisplacement.y, deltaDisplacement.z);
		FASTLOG3F(FLog::HumanoidFloorProcess, "character Position in Object Space: %4.4f, %4.4f, %4.4f", charPosInObjectSpace.x, charPosInObjectSpace.y, charPosInObjectSpace.z);
		FASTLOG3F(FLog::HumanoidFloorProcess, "New characterPosition in Object Space: %4.4f, %4.4f, %4.4f", newCharPosInObjectSpace.x, newCharPosInObjectSpace.y, newCharPosInObjectSpace.z);
	}
}

Vector3 getMaxAllowedDisplacement(const Vector3& floorVelocity, const float& netDt)
{
	return (floorVelocity) * netDt * (float)(DFInt::HumanoidFloorTeleportWeightValue/10.0f);
}

bool Humanoid::validateNetworkUpdateDistance(PartInstance* floorPart, CoordinateFrame& previousFloorPosition, float& netDt)
{
	float bufferMoveDistanceSqr(25.0f);     // Allows some motion when we think the platform is not moving.
	Vector3 calculatedMaximumNetworkUpdate = getMaxAllowedDisplacement(floorPart->getVelocity().linear, netDt);
	Vector3 floorPositionDelta = floorPart->getCoordinateFrame().translation - previousFloorPosition.translation; 

	if (floorPositionDelta.squaredLength() < (calculatedMaximumNetworkUpdate.squaredLength() + bufferMoveDistanceSqr))
		return true;
	else
	{
		FASTLOG3F(FLog::HumanoidFloorProcess, "Detected Teleport with floorPositionDelta: %4.4f, %4.4f, %4.4f", floorPositionDelta.x, floorPositionDelta.y, floorPositionDelta.z);
		FASTLOG1F(FLog::HumanoidFloorProcess, "Maximum 'Teleport' Allowed: %4.4f", calculatedMaximumNetworkUpdate.magnitude());
		return false;
	}
}

void Humanoid::updateFloorSimPhaseCharVelocity(Primitive* floorPrim)
{
	if (floorPrim && primitiveIsLastFloor(floorPrim))
	{
		Assembly* floorAssembly = floorPrim->getAssembly();
		if (floorAssembly)
		{
			int currentPhase = getCurrentFloorFilterPhase();
			// If we suddenly when from not simulating to BufferZone (which happens often) we have to quickly speed the character up by the floor's  velocity.
			if (currentPhase != lastFilterPhase)
			{
				Primitive* torsoPrim = getTorsoPrimitiveFast();
				if ( torsoPrim && getWorld() )
				{
					if ((lastFilterPhase >= (int)Assembly::NoSim_SendIfSim && currentPhase < (int)Assembly::NoSim_SendIfSim))
					{
						FASTLOG(FLog::HumanoidFloorProcess, "Updating Character Velocity based on floor Phase switch");
						torsoPrim->setVelocity(torsoPrim->getPV().velocity + getHumanoidRelativeVelocityToPart(floorPrim));
					}
					else if ((lastFilterPhase < (int)Assembly::NoSim_SendIfSim && currentPhase >= (int)Assembly::NoSim_SendIfSim))
					{
						FASTLOG(FLog::HumanoidFloorProcess, "Updating Character Velocity based on floor Phase switch");
						torsoPrim->setVelocity(torsoPrim->getPV().velocity - getHumanoidRelativeVelocityToPart(floorPrim));
					}
				}
			}
			lastFilterPhase = currentPhase;
		}
	}
}


bool Humanoid::shouldNotApplyFloorVelocity(Primitive* floorPrim)
{
	if (floorPrim && floorPrim->getAssembly() && floorPrim->getWorld() && primitiveIsLastFloor(floorPrim))
	{
		return (getCurrentFloorFilterPhase() >= Assembly::NoSim_SendIfSim && !floorPrim->getAnchoredProperty() );
	}
	return false;
}

bool Humanoid::primitiveIsLastFloor(Primitive* prim)
{
	shared_ptr<PartInstance> savedFloorPart = getLastFloor();

	Assembly* savedFloorAssembly = NULL;
	if (savedFloorPart && savedFloorPart->getPartPrimitive())
	{
		savedFloorAssembly = savedFloorPart->getPartPrimitive()->getAssembly();
	}

	Assembly* checkPrimAssembly = NULL;
	if (prim)
	{
		checkPrimAssembly = prim->getAssembly();
	}

	if (savedFloorAssembly && checkPrimAssembly && savedFloorAssembly->getAssemblyPrimitive() && checkPrimAssembly->getAssemblyPrimitive())
	{
		return true;
	}
	return false;
}

const Velocity Humanoid::getHumanoidRelativeVelocityToPart(Primitive* floorPrim)
{
	RBXASSERT(getWorld());

	if (floorPrim) 
	{
		RBXASSERT(floorPrim->getWorld());
		if (floorPrim->getWorld())
		{

			World* world = getWorld();
			Velocity throttledV = floorPrim->getPV().velocityAtPoint(getRootBodyFast()->getCoordinateFrame().translation);		// this velocity could be inaccurate
			float throttle = world->getEnvironmentSpeed();

			throttledV.linear *= throttle;
			throttledV.rotational *= throttle;

			return throttledV;
		}
	}
	return Velocity::zero();
}

int Humanoid::getCurrentFloorFilterPhase()
{
	if (lastFloorPart.get())
	{
		if (rootFloorMechPart.get())
		{
			Assembly* rootAssembly = rootFloorMechPart->getPartPrimitive()->getAssembly();
			if (rootAssembly)
			{
				return rootAssembly->getFilterPhase();
			}
		}
		// If no root Assembly, fall back to using floorAssembly
		Assembly* floorAssembly = lastFloorPart->getPartPrimitive()->getAssembly();
		return (int)(floorAssembly ? floorAssembly->getFilterPhase() : Assembly::NOT_ASSIGNED);
	}
	return (int)Assembly::NOT_ASSIGNED;
}

int Humanoid::getCurrentFloorFilterPhase(Assembly* floorAssembly)
{
	if (lastFloorPart.get())
	{
		if (rootFloorMechPart.get())
		{
			Assembly* rootAssembly = rootFloorMechPart->getPartPrimitive()->getAssembly();
			if (rootAssembly)
			{
				return rootAssembly->getFilterPhase();
			}
		}
		// If no root Assembly, fall back to using floorAssembly
		return (int)(floorAssembly ? floorAssembly->getFilterPhase() : Assembly::NOT_ASSIGNED);
	}
	return (int)Assembly::NOT_ASSIGNED;
}

} // namespace
